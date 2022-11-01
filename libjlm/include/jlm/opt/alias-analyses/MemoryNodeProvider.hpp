/*
 * Copyright 2022 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_OPT_ALIAS_ANALYSES_MEMORYNODEPROVIDER_HPP
#define JLM_OPT_ALIAS_ANALYSES_MEMORYNODEPROVIDER_HPP

#include <jlm/opt/alias-analyses/PointsToGraph.hpp>
#include <jlm/util/HashSet.hpp>

#include <vector>

namespace jlm::aa {

class MemoryNodeProvider {
public:
  virtual
  ~MemoryNodeProvider() noexcept;

  /**
   * Computes the memory nodes that are required at the entry and exit of of a region as well as call node. This method
   * needs to be called before GetRegionEntryNodes(), GetRegionExitNodes(), GetCallEntryNodes(), or GetCallExitNodes()
   * can be used to retrieve the entry or exit nodes.
   *
   * @param rvsdgModule The RVSDG module on which the memory node provision should be performed.
   */
  virtual void
  ProvisionMemoryNodes(const RvsdgModule & rvsdgModule) = 0;

  [[nodiscard]] virtual const PointsToGraph &
  GetPointsToGraph() const noexcept = 0;

  [[nodiscard]] virtual const HashSet<const PointsToGraph::MemoryNode*> &
  GetRegionEntryNodes(const jive::region & region) const = 0;

  [[nodiscard]] virtual const HashSet<const PointsToGraph::MemoryNode*> &
  GetRegionExitNodes(const jive::region & region) const = 0;

  [[nodiscard]] virtual const HashSet<const PointsToGraph::MemoryNode*> &
  GetCallEntryNodes(const CallNode & callNode) const = 0;

  [[nodiscard]] virtual const HashSet<const PointsToGraph::MemoryNode*> &
  GetCallExitNodes(const CallNode & callNode) const = 0;

  [[nodiscard]] virtual HashSet<const PointsToGraph::MemoryNode*>
  GetOutputNodes(const jive::output & output) const = 0;

  [[nodiscard]] virtual const HashSet<const PointsToGraph::MemoryNode*> &
  GetLambdaEntryNodes(const lambda::node & lambdaNode) const
  {
    return GetRegionEntryNodes(*lambdaNode.subregion());
  }

  [[nodiscard]] virtual const HashSet<const PointsToGraph::MemoryNode*> &
  GetLambdaExitNodes(const lambda::node & lambdaNode) const
  {
    return GetRegionExitNodes(*lambdaNode.subregion());
  }

  [[nodiscard]] virtual const HashSet<const PointsToGraph::MemoryNode*> &
  GetThetaEntryExitNodes(const jive::theta_node & thetaNode) const
  {
    auto & entryNodes = GetRegionEntryNodes(*thetaNode.subregion());
    auto & exitNodes = GetRegionExitNodes(*thetaNode.subregion());
    JLM_ASSERT(entryNodes == exitNodes);
    return entryNodes;
  }

  [[nodiscard]] virtual HashSet<const PointsToGraph::MemoryNode*>
  GetGammaEntryNodes(const jive::gamma_node & gammaNode) const
  {
    HashSet<const PointsToGraph::MemoryNode*> allMemoryNodes;
    for (size_t n = 0; n < gammaNode.nsubregions(); n++) {
      auto & subregion = *gammaNode.subregion(n);
      auto & memoryNodes = GetRegionEntryNodes(subregion);
      allMemoryNodes.UnionWith(memoryNodes);
    }

    return allMemoryNodes;
  }

  [[nodiscard]] virtual HashSet<const PointsToGraph::MemoryNode*>
  GetGammaExitNodes(const jive::gamma_node & gammaNode) const
  {
    HashSet<const PointsToGraph::MemoryNode*> allMemoryNodes;
    for (size_t n = 0; n < gammaNode.nsubregions(); n++) {
      auto & subregion = *gammaNode.subregion(n);
      auto & memoryNodes = GetRegionExitNodes(subregion);
      allMemoryNodes.UnionWith(memoryNodes);
    }

    return allMemoryNodes;
  }
};

}

#endif //JLM_OPT_ALIAS_ANALYSES_MEMORYNODEPROVIDER_HPP
