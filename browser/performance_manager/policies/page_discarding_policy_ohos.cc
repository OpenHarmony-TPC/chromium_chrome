// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/performance_manager/policies/page_discarding_policy_ohos.h"

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "chrome/browser/performance_manager/policies/page_discarding_helper.h"
#include "chrome/browser/ui/browser_list.h"
#include "content/public/browser/browser_thread.h"
#include "base/memory/low_memory_mode_ohos.h"

using content::BrowserThread;
using MemoryPressureLevel = base::MemoryPressureListener::MemoryPressureLevel;

namespace performance_manager::policies {

namespace {

// default
// Keep maximum alive tab count, will discard tab if overflow.
constexpr int32_t kDefaultMaxAliveTabCount = 80;
// Keep maximum audible tab count.
constexpr int32_t kDefaultMaxAudibleTabCount = 8;
// Each time discard count, avoid spending too much time.
constexpr int32_t kDefaultDiscardTabBatchCount = 5;
constexpr int32_t kDefaultDiscardAudibleTabCount = 2;
constexpr int32_t kDefaultDiscardMemoryCountUrgent = 10;
constexpr int32_t kDefaultReservedMemoryCountUrgent = 20;

// LowMemoryMode
constexpr int32_t kLowMemoryMaxAliveTabCount = 15;
constexpr int32_t kLowMemoryMaxAudibleTabCount = 1;
constexpr int32_t kLowMemoryDiscardTabBatchCount = 1;
constexpr int32_t kLowMemoryDiscardAudibleTabCount = 1;
constexpr int32_t kLowMemoryDiscardMemoryCountUrgent = 3;
constexpr int32_t kLowMemoryReservedMemoryCountUrgent = 5;

// Next time duration to discard tab.
constexpr int64_t kDefaultDiscardTimeDeltaMs = 30000;  // 30s
constexpr int64_t kFastDiscardTimeDelatMs = 1000; // 1s

PageNodeVector GetSortedPageNodeVector(const PageNodeSet& page_node_set) {
  PageNodeVector sorted_page_nodes(page_node_set.begin(),
                                   page_node_set.end());
  // Sort lifecycle_units with ascending importance.
  std::sort(sorted_page_nodes.begin(), sorted_page_nodes.end(),
            [](const PageNode* a, const PageNode* b) {
               return a->GetLastVisibilityChangeTime() >
                      b->GetLastVisibilityChangeTime();
            });
  return sorted_page_nodes;
}

}  // namespace

PageDiscardingPolicyOhos::PageDiscardingPolicyOhos() {
  InitializeConfig();
}

PageDiscardingPolicyOhos::~PageDiscardingPolicyOhos() {}

void PageDiscardingPolicyOhos::InitializeConfig() {
   bool low_memory_mode = base::GetLowMemoryMode();
   if (low_memory_mode) {
    max_alive_tab_count_ = kLowMemoryMaxAliveTabCount;
    max_audible_tab_count_ = kLowMemoryMaxAudibleTabCount;
    discard_tab_batch_count_ = kLowMemoryDiscardTabBatchCount;
    discard_audible_tab_count_ = kLowMemoryDiscardAudibleTabCount;
    discard_memory_count_urgent_ = kLowMemoryDiscardMemoryCountUrgent;
    reserved_memory_count_urgent_ = kLowMemoryReservedMemoryCountUrgent;
  } else {
    max_alive_tab_count_ = kDefaultMaxAliveTabCount;
    max_audible_tab_count_ = kDefaultMaxAudibleTabCount;
    discard_tab_batch_count_ = kDefaultDiscardTabBatchCount;
    discard_audible_tab_count_ = kDefaultDiscardAudibleTabCount;
    discard_memory_count_urgent_ = kDefaultDiscardMemoryCountUrgent;
    reserved_memory_count_urgent_ = kDefaultReservedMemoryCountUrgent;
  }
  LOG(INFO) << "PageDiscardingPolicyOhos initialized in "
            << (low_memory_mode ? "LOW MEMORY" : "NORMAL")
            << " mode: max_alive_tab_count=" << max_alive_tab_count_
            << ", max_audible_tab_count=" << max_audible_tab_count_
            << ", discard_tab_batch_count=" << discard_tab_batch_count_;
}

void PageDiscardingPolicyOhos::OnPageNodeAdded(const PageNode* page_node) {
  if (page_node->GetType() == PageType::kTab && !page_node->IsVisible()) {
    // Some mechanisms (like "session restore" and "open all bookmarks") can
    // create pages that are non-visible. If that happens, start a discard
    // timer so that the pages are discarded if they don't ever become
    // visible.
    {
      base::AutoLock auto_lock(lock_);
      candidate_discarding_page_nodes_.insert(page_node);
    }
    StartDiscardTimerIfNeeded();
  }
}

void PageDiscardingPolicyOhos::OnBeforePageNodeRemoved(
    const PageNode* page_node) {
  base::AutoLock auto_lock(lock_);
  candidate_discarding_page_nodes_.erase(page_node);
}

void PageDiscardingPolicyOhos::OnIsAudibleChanged(const PageNode* page_node) {
  if (page_node->GetType() != PageType::kTab) {
    return;
  }

  if (page_node->IsAudible()) {
    {
      base::AutoLock auto_lock(lock_);
      candidate_audible_page_nodes_.insert(page_node);
    }
    PostAudibleTabDiscardTaskIfNeeded();
  } else {
    base::AutoLock auto_lock(lock_);
    candidate_audible_page_nodes_.erase(page_node);
  }
}

void PageDiscardingPolicyOhos::OnIsVisibleChanged(const PageNode* page_node) {
  if (page_node->GetType() != PageType::kTab) {
    return;
  }

  // If the page is made visible, remove it from candidate set; and when it
  // become invisible, add it into candidate set again.
  if (page_node->IsVisible()) {
    base::AutoLock auto_lock(lock_);
    candidate_discarding_page_nodes_.erase(page_node);
  } else {
    {
      base::AutoLock auto_lock(lock_);
      candidate_discarding_page_nodes_.insert(page_node);
    }
    StartDiscardTimerIfNeeded();
  }
}

void PageDiscardingPolicyOhos::OnTypeChanged(const PageNode* page_node,
                                             PageType previous_type) {
  if (page_node->GetType() != PageType::kTab) {
    base::AutoLock auto_lock(lock_);
    candidate_discarding_page_nodes_.erase(page_node);
  }
}

void PageDiscardingPolicyOhos::OnPassedToGraph(Graph * graph) {
  graph_ = graph;
  graph->AddSystemNodeObserver(this);
  graph->AddPageNodeObserver(this);
}

void PageDiscardingPolicyOhos::OnTakenFromGraph(Graph * graph) {
  // The logic in this class depends on being notified of pages being removed,
  // otherwise there's no guarantee PageNode pointers are still valid when
  // timers fire. To avoid possibly having callbacks manipulate invalid
  // PageNode pointers, clear all the existing timers before unregistering the
  // observer.
  graph->RemoveSystemNodeObserver(this);
  graph->RemovePageNodeObserver(this);
  graph_ = nullptr;
}

void PageDiscardingPolicyOhos::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel new_level) {
  if (new_level == MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL) {
    PostMemoryDiscardTask(DiscardReason::kMemoryOverUrgent);
  }
}

void PageDiscardingPolicyOhos::StartDiscardTimerIfNeeded() {
  // Discard threshhold：MAX tab count + window count
  int32_t window_count = BrowserList::GetInstance()->size();
  int32_t tab_count = 0;

  {
    base::AutoLock auto_lock(lock_);
    tab_count = candidate_discarding_page_nodes_.size();
  }

  if (tab_count < max_alive_tab_count_ + window_count) {
    if (tab_discard_timer_) {
      tab_discard_timer_->Stop();
      tab_discard_timer_.reset();
    }
    return;
  }

  if (!tab_discard_timer_) {
    tab_discard_timer_ = std::make_unique<base::RepeatingTimer>();
    tab_discard_timer_->Start(
        FROM_HERE, base::Milliseconds(kDefaultDiscardTimeDeltaMs),
        base::BindRepeating(&PageDiscardingPolicyOhos::DiscardTabByReason,
                            weak_ptr_factory_.GetWeakPtr(),
                            DiscardReason::kTabCountOverflow));
  }
}

void PageDiscardingPolicyOhos::StopDiscardTimerIfNeeded() {
  int32_t window_count = BrowserList::GetInstance()->size();
  int32_t tab_count = 0;

  {
    base::AutoLock auto_lock(lock_);
    tab_count = candidate_discarding_page_nodes_.size();
  }
  if (tab_discard_timer_ && (tab_count < max_alive_tab_count_ + window_count + discard_tab_batch_count_)) {
    tab_discard_timer_->Stop();
    tab_discard_timer_.reset();
  }
}

void PageDiscardingPolicyOhos::PostAudibleTabDiscardTaskIfNeeded() {
  int32_t audible_tab_count = 0;

  {
    base::AutoLock auto_lock(lock_);
    audible_tab_count = candidate_audible_page_nodes_.size();
  }

  if (audible_tab_count >= max_audible_tab_count_) {
    content::GetUIThreadTaskRunner({base::TaskPriority::BEST_EFFORT})->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&PageDiscardingPolicyOhos::DiscardTabByReason,
                       weak_ptr_factory_.GetWeakPtr(),
                       DiscardReason::kAudibleTabOverflow),
        base::Milliseconds(kFastDiscardTimeDelatMs));
  }
}

void PageDiscardingPolicyOhos::PostMemoryDiscardTask(DiscardReason reason) {
  content::GetUIThreadTaskRunner({base::TaskPriority::BEST_EFFORT})->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&PageDiscardingPolicyOhos::DiscardTabByReason,
                     weak_ptr_factory_.GetWeakPtr(), reason),
      base::Milliseconds(kFastDiscardTimeDelatMs));
}

void PageDiscardingPolicyOhos::DiscardTabByReason(DiscardReason reason) {
  PageNodeVector sorted_page_node;
  int32_t count_to_discard = 0;
  switch (reason) {
    case DiscardReason::kTabCountOverflow:
      {
        base::AutoLock auto_lock(lock_);
        sorted_page_node =
            GetSortedPageNodeVector(candidate_discarding_page_nodes_);
      }
      count_to_discard = discard_tab_batch_count_;
      break;
    case DiscardReason::kAudibleTabOverflow:
      {
        base::AutoLock auto_lock(lock_);
        sorted_page_node = GetSortedPageNodeVector(candidate_audible_page_nodes_);
      }
      count_to_discard = discard_audible_tab_count_;
      break;
    case DiscardReason::kMemoryOverUrgent:
      {
        base::AutoLock auto_lock(lock_);
        sorted_page_node =
            GetSortedPageNodeVector(candidate_discarding_page_nodes_);
        auto tab_count = sorted_page_node.size();
        count_to_discard = discard_memory_count_urgent_;
        if (tab_count - count_to_discard < reserved_memory_count_urgent_) {
          count_to_discard = tab_count - reserved_memory_count_urgent_;
        }
        if (count_to_discard <= 0) {
          LOG(INFO) << "Tab count reaches the reserved threshold. Don't discard.";
          return;
        }
        break;
      }
    default:
      return;
  }
  DiscardTabImpl(reason, sorted_page_node, count_to_discard);
}

void PageDiscardingPolicyOhos::DiscardTabImpl(DiscardReason reason,
                                              PageNodeVector nodes,
                                              int32_t total_count) {
  int32_t discard_count = 0;
  // min of total count and discard count to be removed
  PageNodeVector discard_vector;
  for (const PageNode* page_node : nodes) {
    discard_vector.push_back(page_node);
    if (++discard_count >= total_count) {
      break;
    }
  }
  auto iter1 = std::begin(nodes);
  PageDiscardingHelper::GetFromGraph(graph_)->ImmediatelyDiscardMultiplePages(
      discard_vector, DiscardEligibilityPolicy::DiscardReason::EXTERNAL);
  LOG(WARNING) << "DiscardTabImpl reason: " << reason << ", total candidate tab: "
               << nodes.size() << ", discard count: " << discard_count;
  StopDiscardTimerIfNeeded();
}

}  // namespace performance_manager::policies
