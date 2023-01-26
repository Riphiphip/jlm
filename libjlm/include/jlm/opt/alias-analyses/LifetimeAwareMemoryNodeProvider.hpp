/*
 * Copyright 2023 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_LIFETIMEAWAREMEMORYNODEPROVIDER_HPP
#define JLM_LIFETIMEAWAREMEMORYNODEPROVIDER_HPP

#include <jlm/opt/alias-analyses/MemoryNodeProvider.hpp>
#include <jlm/util/Statistics.hpp>

namespace jlm::aa {

class LifetimeAwareMemoryNodeProvisioning;

class LifetimeAwareMemoryNodeProvider final : public MemoryNodeProvider
{
  class Context;

public:
  ~LifetimeAwareMemoryNodeProvider() noexcept override;

  LifetimeAwareMemoryNodeProvider() = default;

  LifetimeAwareMemoryNodeProvider(const LifetimeAwareMemoryNodeProvider&) = delete;

  LifetimeAwareMemoryNodeProvider(LifetimeAwareMemoryNodeProvider&&) = delete;

  LifetimeAwareMemoryNodeProvider&
  operator=(const LifetimeAwareMemoryNodeProvider&) = delete;

  LifetimeAwareMemoryNodeProvider&
  operator=(LifetimeAwareMemoryNodeProvider&&) = delete;

  std::unique_ptr<MemoryNodeProvisioning>
  ProvisionMemoryNodes(
    const RvsdgModule & rvsdgModule,
    const PointsToGraph & pointsToGraph,
    StatisticsCollector & statisticsCollector) override;

  /**
   * Creates a LifetimeAwareMemoryNodeProvider and calls the ProvisionMemoryNodes() method.
   *
   * @param rvsdgModule The RVSDG module on which the provisioning should be performed.
   * @param pointsToGraph The PointsToGraph corresponding to the RVSDG module.
   * @param statisticsCollector The statistics collector for collecting pass statistics.
   *
   * @return A new instance of MemoryNodeProvisioning.
   */
  static std::unique_ptr<MemoryNodeProvisioning>
  Create(
    const RvsdgModule & rvsdgModule,
    const PointsToGraph & pointsToGraph,
    StatisticsCollector & statisticsCollector);

  /**
   * Creates a LifetimeAwareMemoryNodeProvider and calls the ProvisionMemoryNodes() method.
   *
   * @param rvsdgModule The RVSDG module on which the provisioning should be performed.
   * @param pointsToGraph The PointsToGraph corresponding to the RVSDG module.
   * @param statisticsCollector The statistics collector for collecting pass statistics.
   *
   * @return A new instance of MemoryNodeProvisioning.
   */
  static std::unique_ptr<MemoryNodeProvisioning>
  Create(
    const RvsdgModule & rvsdgModule,
    const PointsToGraph & pointsToGraph);

private:
  void
  AnnotateTopLifetime(const RvsdgModule & rvsdgModule);

  void
  AnnotateTopLifetimeRegion(jive::region & region);

  void
  AnnotateTopLifetimeStructuralNode(const jive::structural_node & structuralNode);

  void
  AnnotateTopLifetimeLambda(const lambda::node & lambdaNode);

  void
  AnnotateTopLifetimeSimpleNode(const jive::simple_node & simpleNode);

  void
  AnnotateTopLifetimeAlloca(const jive::simple_node & node);

  void
  AnnotateTopLifetimeStore(const StoreNode & storeNode);

  static bool
  IsAllocaNode(const PointsToGraph::MemoryNode * memoryNode) noexcept;

  std::unique_ptr<Context> Context_;
};

};

#endif //JLM_LIFETIMEAWAREMEMORYNODEPROVIDER_HPP