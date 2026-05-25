// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERFORMANCE_MANAGER_POLICIES_PAGE_DISCARDING_POLICY_OHOS_H_
#define CHROME_BROWSER_PERFORMANCE_MANAGER_POLICIES_PAGE_DISCARDING_POLICY_OHOS_H_

#include <map>
#include <memory>

#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "components/performance_manager/public/graph/graph.h"
#include "components/performance_manager/public/graph/page_node.h"
#include "components/performance_manager/public/graph/system_node.h"

namespace performance_manager::policies {

using PageNodeSet = base::flat_set<const PageNode*>;
using PageNodeVector = std::vector<const PageNode*>;

// This policy is responsible for discarding inactive tabs on ohos:
// Case 1: tab number overflow;
// Case 2: tab number using audio/video overflow;
// Case 3: Memory usage overflow;
class PageDiscardingPolicyOhos : public GraphOwned,
                                 public PageNodeObserver,
                                 public SystemNodeObserver {
 public:
  PageDiscardingPolicyOhos();
  ~PageDiscardingPolicyOhos() override;

  // PageNodeObserver implementation:
  void OnPageNodeAdded(const PageNode* page_node) override;
  void OnBeforePageNodeRemoved(const PageNode* page_node) override;
  void OnIsAudibleChanged(const PageNode* page_node) override;
  void OnIsVisibleChanged(const PageNode* page_node) override;
  void OnTypeChanged(const PageNode* page_node,
                     PageType previous_type) override;

  // GraphOwned implementation:
  void OnPassedToGraph(Graph* graph) override;
  void OnTakenFromGraph(Graph* graph) override;

  // SystemNodeObserver implementation:
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel new_level) override;

 private:
  // enum discard type
  enum DiscardReason {
    kTabCountOverflow = 0,
    kAudibleTabOverflow,
    kMemoryOverModerate,
    kMemoryOverUrgent,
  };

  int32_t max_alive_tab_count_ = 80;
  int32_t max_audible_tab_count_ = 8;
  int32_t discard_tab_batch_count_ = 4;
  int32_t discard_audible_tab_count_ = 2;
  int32_t discard_memory_count_urgent_ = 10;
  int32_t reserved_memory_count_urgent_ = 20;

  void StartDiscardTimerIfNeeded();
  void StopDiscardTimerIfNeeded();
  void InitializeConfig();
  void PostAudibleTabDiscardTaskIfNeeded();
  void PostMemoryDiscardTask(DiscardReason reason);
  void DiscardTabByReason(DiscardReason reason);
  void DiscardTabImpl(DiscardReason reason,
                      PageNodeVector nodes,
                      int32_t total_count);

  base::Lock lock_;
  PageNodeSet GUARDED_BY(lock_) candidate_discarding_page_nodes_;
  PageNodeSet GUARDED_BY(lock_) candidate_audible_page_nodes_;
  std::unique_ptr<base::RepeatingTimer> tab_discard_timer_;

  raw_ptr<Graph> graph_ = nullptr;
  base::WeakPtrFactory<PageDiscardingPolicyOhos> weak_ptr_factory_{this};
};

}  // namespace performance_manager::policies

#endif  // CHROME_BROWSER_PERFORMANCE_MANAGER_POLICIES_PAGE_DISCARDING_POLICY_OHOS_H_
