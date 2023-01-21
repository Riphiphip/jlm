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
  std::unique_ptr<MemoryNodeProvisioning>
  ProvisionMemoryNodes(
    const RvsdgModule & rvsdgModule,
    StatisticsCollector & statisticsCollector);

  void
  HandleLambda(const lambda::node & lambdaNode);

  /**
   * TODO: The RegionAwareMemoryNodeProvider features the same function. Move this function to the Rvsdg class.
   *
   * Extracts all tail nodes of the RVSDG root region.
   *
   * A tail node is any node in the root region on which no other node in the root region depends on. An example would
   * be a lambda node that is not called within the RVSDG module.
   *
   * @param rvsdgModule The RVSDG module from which to extract the tail nodes.
   * @return A vector of tail nodes.
   */
  static std::vector<const jive::node*>
  ExtractRvsdgTailNodes(const RvsdgModule & rvsdgModule);

  static bool
  IsTailNode(const jive::node & node);

  static bool
  IsOnlyExported(const jive::output & output);

  std::unique_ptr<LifetimeAwareMemoryNodeProvisioning> Provisioning_;
};

};

#endif //JLM_LIFETIMEAWAREMEMORYNODEPROVIDER_HPP