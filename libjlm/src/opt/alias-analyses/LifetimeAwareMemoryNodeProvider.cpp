/*
 * Copyright 2023 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/opt/alias-analyses/AgnosticMemoryNodeProvider.hpp>
#include <jlm/opt/alias-analyses/LifetimeAwareMemoryNodeProvider.hpp>

#include <jive/rvsdg/traverser.hpp>

namespace jlm::aa {

class LifetimeAwareMemoryNodeProvisioning final : public MemoryNodeProvisioning
{
public:
  explicit
  LifetimeAwareMemoryNodeProvisioning(const PointsToGraph & pointsToGraph)
    : PointsToGraph_(pointsToGraph)
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
    JLM_ASSERT(ContainsRegionEntryNodes(region));
    return regionEntryMemoryNodes_.find(&region)->second;
  }

  [[nodiscard]] const HashSet<const PointsToGraph::MemoryNode*> &
  GetRegionExitNodes(const jive::region & region) const override
  {
    JLM_ASSERT(ContainsRegionExitNodes(region));
    return regionExitMemoryNodes_.find(&region)->second;
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
    JLM_ASSERT(is<PointerType>(output.type()));
    auto & registerNode = PointsToGraph_.GetRegisterNode(output);

    HashSet<const PointsToGraph::MemoryNode*> memoryNodes;
    for (auto & memoryNode : registerNode.Targets())
      memoryNodes.Insert(&memoryNode);

    return memoryNodes;
  }

  void
  AddRegionEntryNodes(
    const jive::region & region,
    HashSet<const PointsToGraph::MemoryNode*> memoryNodes)
  {
    JLM_ASSERT(!ContainsRegionEntryNodes(region));
    regionEntryMemoryNodes_[&region] = std::move(memoryNodes);
  }

  void
  AddRegionExitNodes(
    const jive::region & region,
    HashSet<const PointsToGraph::MemoryNode*> memoryNodes)
  {
    JLM_ASSERT(!ContainsRegionExitNodes(region));
    regionExitMemoryNodes_[&region] = std::move(memoryNodes);
  }

  static std::unique_ptr<LifetimeAwareMemoryNodeProvisioning>
  Create(const PointsToGraph & pointsToGraph)
  {
    return std::make_unique<LifetimeAwareMemoryNodeProvisioning>(pointsToGraph);
  }

private:
  [[nodiscard]] bool
  ContainsRegionEntryNodes(const jive::region & region) const noexcept
  {
    return regionEntryMemoryNodes_.find(&region) != regionEntryMemoryNodes_.end();
  }

  [[nodiscard]] bool
  ContainsRegionExitNodes(const jive::region & region) const noexcept
  {
    return regionExitMemoryNodes_.find(&region) != regionExitMemoryNodes_.end();
  }

  const PointsToGraph & PointsToGraph_;

  std::unordered_map<const jive::region*, HashSet<const PointsToGraph::MemoryNode*>> regionEntryMemoryNodes_;
  std::unordered_map<const jive::region*, HashSet<const PointsToGraph::MemoryNode*>> regionExitMemoryNodes_;
};

/** \brief Context for lifetime aware provisioning
 *
 */
class LifetimeAwareMemoryNodeProvider::Context final
{
public:
  explicit
  Context(const MemoryNodeProvisioning & seedProvisioning)
    : SeedProvisioning_(seedProvisioning)
    , Provisioning_(LifetimeAwareMemoryNodeProvisioning::Create(seedProvisioning.GetPointsToGraph()))
  {}

  Context(const Context&) = delete;

  Context(Context&&) noexcept = delete;

  Context&
  operator=(const Context&) = delete;

  Context&
  operator=(Context&&) noexcept = delete;

  [[nodiscard]] const MemoryNodeProvisioning &
  GetSeedProvisioning() const noexcept
  {
    return SeedProvisioning_;
  }

  [[nodiscard]] const PointsToGraph &
  GetPointsToGraph() const noexcept
  {
    return GetSeedProvisioning().GetPointsToGraph();
  }

  [[nodiscard]] std::unique_ptr<LifetimeAwareMemoryNodeProvisioning>
  GetLifetimeAwareMemoryNodeProvisioning() noexcept
  {
    return std::move(Provisioning_);
  }

  [[nodiscard]] const lambda::node::CallSummary &
  GetCallSummary(const lambda::node & lambdaNode)
  {
    if (HasCallSummary(lambdaNode))
    {
      return *CallSummaries_.find(&lambdaNode)->second;
    }

    auto callSummary = lambdaNode.ComputeCallSummary();
    CallSummaries_[&lambdaNode] = std::move(callSummary);
    return *CallSummaries_.find(&lambdaNode)->second;
  }

  void
  AddAliveNode(
    const jive::region & region,
    const PointsToGraph::MemoryNode & memoryNode)
  {
    auto aliveNodes = GetAliveNodes(region);
    aliveNodes.Insert(&memoryNode);
  }

  void
  AddAliveNodes(
    const jive::region & region,
    const HashSet<const PointsToGraph::MemoryNode*> & memoryNodes)
  {
    auto aliveNodes = GetAliveNodes(region);
    aliveNodes.UnionWith(memoryNodes);
  }

  void
  AddRegionEntryNodes(
    const jive::region & region,
    HashSet<const PointsToGraph::MemoryNode*> & memoryNodes)
  {
    Provisioning_->AddRegionEntryNodes(region, std::move(memoryNodes));
  }

  void
  AddRegionExitNodes(
    const jive::region & region,
    HashSet<const PointsToGraph::MemoryNode*> & memoryNodes)
  {
    Provisioning_->AddRegionExitNodes(region, std::move(memoryNodes));
  }

  static std::unique_ptr<Context>
  Create(const MemoryNodeProvisioning & seedProvisioning)
  {
    return std::make_unique<Context>(seedProvisioning);
  }

private:
  bool
  HasAliveNodesSet(const jive::region & region) const noexcept
  {
    return AliveNodes_.find(&region) != AliveNodes_.end();
  }

  HashSet<const PointsToGraph::MemoryNode*> &
  GetAliveNodes(const jive::region & region)
  {
    if (!HasAliveNodesSet(region))
    {
      AliveNodes_[&region] = {};
    }

    return AliveNodes_[&region];
  }

  [[nodiscard]] bool
  HasCallSummary(const lambda::node & lambdaNode)
  {
    return CallSummaries_.find(&lambdaNode) != CallSummaries_.end();
  }

  const MemoryNodeProvisioning & SeedProvisioning_;
  std::unique_ptr<LifetimeAwareMemoryNodeProvisioning> Provisioning_;

  /**
   * Keeps track of the memory nodes that are alive within a region.
   */
  std::unordered_map<const jive::region*, HashSet<const PointsToGraph::MemoryNode*>> AliveNodes_;

  std::unordered_map<const lambda::node*, std::unique_ptr<lambda::node::CallSummary>> CallSummaries_;
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
  Context_ = LifetimeAwareMemoryNodeProvider::Context::Create(*seedProvisioning);

  /*
   * FIXME: add statistics
   */
  AnnotateTopLifetime(rvsdgModule);

  return Context_->GetLifetimeAwareMemoryNodeProvisioning();
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

void
LifetimeAwareMemoryNodeProvider::AnnotateTopLifetime(const RvsdgModule &rvsdgModule)
{
  jive::bottomup_traverser traverser(rvsdgModule.Rvsdg().root());
  for (auto & node : traverser)
  {
    if (auto lambdaNode = dynamic_cast<const lambda::node*>(node))
    {
      AnnotateTopLifetimeLambda(*lambdaNode);
    }
    else if (auto phiNode = dynamic_cast<const phi::node*>(node))
    {
      JLM_UNREACHABLE("Not yet implemented!");
    }
    else if (dynamic_cast<const delta::node*>(node))
    {
      /*
       * Nothing needs to be done.
       */
    }
    else
    {
      JLM_UNREACHABLE("Unhandled node type!");
    }
  }
}

void
LifetimeAwareMemoryNodeProvider::AnnotateTopLifetimeRegion(jive::region & region)
{
  jive::topdown_traverser traverser(&region);
  for (auto & node : traverser)
  {
    if (auto simpleNode = dynamic_cast<const jive::simple_node*>(node))
    {
      AnnotateTopLifetimeSimpleNode(*simpleNode);
    }
    else if (auto structuralNode = dynamic_cast<const jive::structural_node*>(node))
    {
      AnnotateTopLifetimeStructuralNode(*structuralNode);
    }
    else
    {
      JLM_UNREACHABLE("Unhandled node type!");
    }
  }
}

void
LifetimeAwareMemoryNodeProvider::AnnotateTopLifetimeStructuralNode(const jive::structural_node & structuralNode)
{
  JLM_UNREACHABLE("Not yet implemented!");
}

void
LifetimeAwareMemoryNodeProvider::AnnotateTopLifetimeLambda(const lambda::node & lambdaNode)
{
  auto annotateLambdaEntry = [&](const lambda::node & lambdaNode)
  {
    auto & lambdaRegion = *lambdaNode.subregion();

    auto & callSummary = Context_->GetCallSummary(lambdaNode);
    if (callSummary.IsDead())
    {
      JLM_UNREACHABLE("Not yet implemented!");
    }
    else if (callSummary.IsOnlyExported())
    {
      auto memoryNodes = Context_->GetSeedProvisioning().GetLambdaEntryNodes(lambdaNode);
      memoryNodes.RemoveWhere(IsAllocaNode);
      Context_->AddAliveNodes(lambdaRegion, memoryNodes);
      Context_->AddRegionEntryNodes(lambdaRegion, memoryNodes);
    }
    else if (callSummary.HasOnlyDirectCalls())
    {
      JLM_UNREACHABLE("Not yet implemented!");
    }
    else
    {
      JLM_UNREACHABLE("Not yet implemented!");
    }
  };

  auto annotateLambdaExit = [&](const lambda::node & lambdaNode)
  {
    auto & lambdaRegion = *lambdaNode.subregion();

    auto & callSummary = Context_->GetCallSummary(lambdaNode);
    if (callSummary.IsDead())
    {
      JLM_UNREACHABLE("Not yet implemented!");
    }
    else if (callSummary.IsOnlyExported())
    {
      auto memoryNodes = Context_->GetSeedProvisioning().GetLambdaEntryNodes(lambdaNode);
      memoryNodes.RemoveWhere(IsAllocaNode);
      Context_->AddRegionExitNodes(lambdaRegion, memoryNodes);
    }
    else if (callSummary.HasOnlyDirectCalls())
    {
      JLM_UNREACHABLE("Not yet implemented!");
    }
    else
    {
      JLM_UNREACHABLE("Not yet implemented!");
    }
  };

  annotateLambdaEntry(lambdaNode);
  AnnotateTopLifetimeRegion(*lambdaNode.subregion());
  annotateLambdaExit(lambdaNode);
}

void
LifetimeAwareMemoryNodeProvider::AnnotateTopLifetimeSimpleNode(const jive::simple_node & simpleNode)
{
  auto annotateAlloca = [](auto & p, auto & n) { p.AnnotateTopLifetimeAlloca(n); };

  static std::unordered_map<
    std::type_index,
    std::function<void(LifetimeAwareMemoryNodeProvider&, const jive::simple_node&)>> nodes
    ({
       {typeid(alloca_op), annotateAlloca}
     });

  auto & operation = simpleNode.operation();
  if (nodes.find(typeid(operation)) != nodes.end())
  {
    nodes[typeid(operation)](*this, simpleNode);
  }
}

void
LifetimeAwareMemoryNodeProvider::AnnotateTopLifetimeAlloca(const jive::simple_node & node)
{
  JLM_ASSERT(is<alloca_op>(&node));

  auto & allocaNode = Context_->GetPointsToGraph().GetAllocaNode(node);
  Context_->AddAliveNode(*node.region(), allocaNode);
}

bool
LifetimeAwareMemoryNodeProvider::IsAllocaNode(const PointsToGraph::MemoryNode * memoryNode) noexcept
{
  return PointsToGraph::Node::Is<PointsToGraph::AllocaNode>(*memoryNode);
}

}