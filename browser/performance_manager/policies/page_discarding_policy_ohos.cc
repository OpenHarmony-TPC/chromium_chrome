/*
 * Copyright (c) 2023-2025 Haitai FangYuan Co., Ltd.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "chrome/browser/performance_manager/policies/page_discarding_policy_ohos.h"

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "chrome/browser/performance_manager/policies/page_discarding_helper.h"
#include "chrome/browser/ui/browser_list.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;
using MemoryPressureLevel = base::MemoryPressureListener::MemoryPressureLevel;

namespace performance_manager::policies {

namespace {

// Keep maximum alive tab count, will discard tab if overflow.
constexpr int32_t kMaxAliveTabCount = 80;
// Keep maximum audible tab count.
constexpr int32_t kMaxAudibleTabCount = 8;
// Each time discard count, avoid spending too much time.
constexpr int32_t kDiscardTabBatchCount = 5;
constexpr int32_t kDiscardAudibleTabCount = 2;
constexpr int32_t kDiscardMemoryCountUrgent = 10;
constexpr int32_t kReservedMemoryCountUrgent = 20;
// Next time duration to discard tab.
constexpr int64_t kDefaultDiscardTimeDeltaMs = 30000;  // 30s
constexpr int64_t kFastDiscardTimeDelatMs = 1000; // 1s

PageNodeVector GetSortedPageNodeVector(const PageNodeSet& page_node_set) {
  PageNodeVector sorted_page_nodes(page_node_set.begin(),
                                   page_node_set.end());
  // Sort lifecycle_units with ascending importance.
  std::sort(sorted_page_nodes.begin(), sorted_page_nodes.end(),
            [](const PageNode* a, const PageNode* b) {
               return a->GetTimeSinceLastVisibilityChange() >
                      b->GetTimeSinceLastVisibilityChange();
            });
  return sorted_page_nodes;
}

}  // namespace

PageDiscardingPolicyOhos::PageDiscardingPolicyOhos() {}

PageDiscardingPolicyOhos::~PageDiscardingPolicyOhos() {}

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

  if (tab_count < kMaxAliveTabCount + window_count) {
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

void PageDiscardingPolicyOhos::PostAudibleTabDiscardTaskIfNeeded() {
  int32_t audible_tab_count = 0;

  {
    base::AutoLock auto_lock(lock_);
    audible_tab_count = candidate_audible_page_nodes_.size();
  }

  if (audible_tab_count >= kMaxAudibleTabCount) {
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

void PageDiscardingPolicyOhos::DiscardTabByReason(DiscardReason reson) {
  PageNodeVector sorted_page_node;
  int32_t count_to_discard = 0;
  switch (reson) {
    case DiscardReason::kTabCountOverflow:
      {
        base::AutoLock auto_lock(lock_);
        sorted_page_node =
            GetSortedPageNodeVector(candidate_discarding_page_nodes_);
      }
      count_to_discard = kDiscardTabBatchCount;
      break;
    case DiscardReason::kAudibleTabOverflow:
      {
        base::AutoLock auto_lock(lock_);
        sorted_page_node = GetSortedPageNodeVector(candidate_audible_page_nodes_);
      }
      count_to_discard = kDiscardAudibleTabCount;
      break;
    case DiscardReason::kMemoryOverUrgent:
      {
        base::AutoLock auto_lock(lock_);
        sorted_page_node =
            GetSortedPageNodeVector(candidate_discarding_page_nodes_);
        auto tab_count = sorted_page_node.size();
        count_to_discard = kDiscardMemoryCountUrgent;
        if (tab_count - count_to_discard < kReservedMemoryCountUrgent) {
          count_to_discard = tab_count - kReservedMemoryCountUrgent;
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
  DiscardTabImpl(reson, sorted_page_node, count_to_discard);
}

void PageDiscardingPolicyOhos::DiscardTabImpl(DiscardReason reson,
                                              PageNodeVector nodes,
                                              int32_t total_count) {
  int32_t discard_count = 0;
  // min of total count and discard count to be removed
  PageNodeVector discard_vector{total_count};
  for (const PageNode* page_node : nodes) {
    discard_vector.push_back(page_node);
    if (++discard_count >= total_count) {
      break;
    }
  }
  auto iter1 = std::begin(nodes);
  PageDiscardingHelper::GetFromGraph(graph_)->ImmediatelyDiscardMultiplePages(
      discard_vector, PageDiscardingHelper::DiscardReason::EXTERNAL,
      base::BindOnce([](std::optional<base::TimeTicks> ticks) {
        if (!ticks) {
          LOG(WARNING) << "DiscardByTabNumberOverflow failed!";
        }
      }));
  LOG(WARNING) << "DiscardTabImpl reason: " << reson << ", total candidate tab: "
               << nodes.size() << ", discard count: " << discard_count;
}

}  // namespace performance_manager::policies
