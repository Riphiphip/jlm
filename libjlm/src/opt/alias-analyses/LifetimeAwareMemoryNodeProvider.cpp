/*
 * Copyright 2023 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/opt/alias-analyses/AgnosticMemoryNodeProvider.hpp>
#include <jlm/opt/alias-analyses/LifetimeAwareMemoryNodeProvider.hpp>

namespace jlm::aa {

class LifetimeAwareMemoryNodeProvisioning final : public MemoryNodeProvisioning
{
public:
  explicit
  LifetimeAwareMemoryNodeProvisioning(
    const PointsToGraph & pointsToGraph,
    const MemoryNodeProvisioning & seedProvisioning)
    : PointsToGraph_(pointsToGraph)
    , SeedProvisioning_(seedProvisioning)
  {}

  LifetimeAwareMemoryNodeProvisioning(const LifetimeAwareMemoryNodeProvisioning&) = delete;

  LifetimeAwareMemoryNodeProvisioning(LifetimeAwareMemoryNodeProvisioning&&) = delete;

  LifetimeAwareMemoryNodeProvisioning&
  operator=(const LifetimeAwareMemoryNodeProvisioning&) = delete;

  LifetimeAwareMemoryNodeProvisioning&
  operator=(LifetimeAwareMemoryNodeProvisioning&&) = delete;

  [[nodiscard]] const PointsToGraph &
  GetPointsToGraph() const noexcept override
  {
    return PointsToGraph_;
  }

  [[nodiscard]] const HashSet<const PointsToGraph::MemoryNode*> &
  GetRegionEntryNodes(const jive::region & region) const override
  {
    JLM_UNREACHABLE("Not yet implemented!");
  }

  [[nodiscard]] const HashSet<const PointsToGraph::MemoryNode*> &
  GetRegionExitNodes(const jive::region & region) const override
  {
    JLM_UNREACHABLE("Not yet implemented");
  }

  [[nodiscard]] const HashSet<const PointsToGraph::MemoryNode*> &
  GetCallEntryNodes(const CallNode & callNode) const override
  {
    JLM_UNREACHABLE("Not yet implemented!");
  }

  [[nodiscard]] const HashSet<const PointsToGraph::MemoryNode*> &
  GetCallExitNodes(const CallNode & callNode) const override
  {
    JLM_UNREACHABLE("Not yet implemented!");
  }

  [[nodiscard]] HashSet<const PointsToGraph::MemoryNode*>
  GetOutputNodes(const jive::output & output) const override
  {
    JLM_UNREACHABLE("Not yet implemented!");
  }

  [[nodiscard]] const MemoryNodeProvisioning &
  GetSeedProvisioning() const noexcept
  {
    return SeedProvisioning_;
  }

  static std::unique_ptr<LifetimeAwareMemoryNodeProvisioning>
  Create(
    const PointsToGraph & pointsToGraph,
    const MemoryNodeProvisioning & seedProvisioning)
  {
    return std::make_unique<LifetimeAwareMemoryNodeProvisioning>(
      pointsToGraph,
      seedProvisioning);
  }

private:
  const PointsToGraph & PointsToGraph_;
  const MemoryNodeProvisioning & SeedProvisioning_;
};

LifetimeAwareMemoryNodeProvider::~LifetimeAwareMemoryNodeProvider() noexcept
= default;

std::unique_ptr<MemoryNodeProvisioning>
LifetimeAwareMemoryNodeProvider::ProvisionMemoryNodes(
  const RvsdgModule &rvsdgModule,
  const PointsToGraph &pointsToGraph,
  StatisticsCollector &statisticsCollector)
{
  auto seedProvisioning = AgnosticMemoryNodeProvider::Create(rvsdgModule, pointsToGraph);
  Provisioning_ = LifetimeAwareMemoryNodeProvisioning::Create(pointsToGraph, *seedProvisioning);

  JLM_UNREACHABLE("Not yet implemented!");
}

std::unique_ptr<MemoryNodeProvisioning>
LifetimeAwareMemoryNodeProvider::Create(
  const RvsdgModule &rvsdgModule,
  const PointsToGraph &pointsToGraph,
  StatisticsCollector &statisticsCollector)
{
  LifetimeAwareMemoryNodeProvider provider;
  return provider.ProvisionMemoryNodes(rvsdgModule, pointsToGraph, statisticsCollector);
}

std::unique_ptr<MemoryNodeProvisioning>
LifetimeAwareMemoryNodeProvider::Create(
  const RvsdgModule &rvsdgModule,
  const PointsToGraph &pointsToGraph)
{
  StatisticsCollector statisticsCollector;
  return Create(rvsdgModule, pointsToGraph, statisticsCollector);
}

std::unique_ptr<MemoryNodeProvisioning>
LifetimeAwareMemoryNodeProvider::ProvisionMemoryNodes(
  const RvsdgModule & rvsdgModule,
  StatisticsCollector &statisticsCollector)
{
  auto tailNodes = ExtractRvsdgTailNodes(rvsdgModule);

}

void
LifetimeAwareMemoryNodeProvider::HandleLambda(const lambda::node &lambdaNode)
{
  if (IsTailNode(lambdaNode))
  {
    auto memoryNodes = Provisioning_->GetRegionEntryNodes(*lambdaNode.region());
    for (auto & memoryNode : memoryNodes)
    {
      if ()
    }
  }
}

std::vector<const jive::node*>
LifetimeAwareMemoryNodeProvider::ExtractRvsdgTailNodes(const jlm::RvsdgModule & rvsdgModule)
{
  auto & rootRegion = *rvsdgModule.Rvsdg().root();

  std::vector<const jive::node*> nodes;
  for (auto & node : rootRegion.bottom_nodes)
  {
    nodes.push_back(&node);
  }

  for (size_t n = 0; n < rootRegion.nresults(); n++)
  {
    auto output = rootRegion.result(n)->origin();
    if (IsOnlyExported(*output))
    {
      nodes.push_back(jive::node_output::node(output));
    }
  }

  return nodes;
}

bool
LifetimeAwareMemoryNodeProvider::IsTailNode(const jive::node &node)
{
  for (size_t n = 0; n < node.noutputs(); n++)
  {
    auto output = node.output(n);
    if (!IsOnlyExported(*output))
    {
      return false;
    }
  }

  return true;
}

bool
LifetimeAwareMemoryNodeProvider::IsOnlyExported(const jive::output &output)
{
  auto IsRootRegionExport = [](const jive::input * input)
  {
    if (!input->region()->IsRootRegion())
    {
      return false;
    }

    if (jive::node_input::node(*input))
    {
      return false;
    }

    return true;
  };

  return std::all_of(
    output.begin(),
    output.end(),
    IsRootRegionExport);
}

}