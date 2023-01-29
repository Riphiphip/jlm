/*
 * Copyright 2023 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <test-registry.hpp>
#include <TestRvsdgs.hpp>

#include <jlm/opt/alias-analyses/LifetimeAwareMemoryNodeProvider.hpp>
#include <jlm/opt/alias-analyses/Steensgaard.hpp>

static std::unique_ptr<jlm::aa::PointsToGraph>
RunSteensgaard(jlm::RvsdgModule & rvsdgModule)
{
  jlm::aa::Steensgaard steensgaard;
  return steensgaard.Analyze(rvsdgModule);
}

static void
TestStore1()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](
    const StoreTest1 & test,
    const jlm::aa::MemoryNodeProvisioning & provisioning,
    const jlm::aa::PointsToGraph & pointsToGraph)
  {
    auto & lambdaMemoryNode = pointsToGraph.GetLambdaNode(*test.lambda);
    auto & externalMemoryNode = pointsToGraph.GetExternalMemoryNode();

    jlm::HashSet<const jlm::aa::PointsToGraph::MemoryNode *> expectedMemoryNodes(
      {
        &lambdaMemoryNode,
        &externalMemoryNode
      });

    auto & lambdaEntryNodes = provisioning.GetLambdaEntryNodes(*test.lambda);
    assert(lambdaEntryNodes == expectedMemoryNodes);

    auto & lambdaExitNodes = provisioning.GetLambdaExitNodes(*test.lambda);
    assert(lambdaExitNodes == expectedMemoryNodes);
  };

  StoreTest1 test;
  auto pointsToGraph = RunSteensgaard(test.module());

  /*
   * Act
   */
  auto provisioning = jlm::aa::LifetimeAwareMemoryNodeProvider::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *provisioning, *pointsToGraph);
}

static void
TestStore2()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](
    const StoreTest2 & test,
    const jlm::aa::MemoryNodeProvisioning & provisioning,
    const jlm::aa::PointsToGraph & pointsToGraph)
  {
    auto & lambdaMemoryNode = pointsToGraph.GetLambdaNode(*test.lambda);
    auto & externalMemoryNode = pointsToGraph.GetExternalMemoryNode();

    jlm::HashSet<const jlm::aa::PointsToGraph::MemoryNode*> expectedMemoryNodes(
      {
        &lambdaMemoryNode,
        &externalMemoryNode
      });

    auto & lambdaEntryNodes = provisioning.GetLambdaEntryNodes(*test.lambda);
    assert(lambdaEntryNodes == expectedMemoryNodes);

    auto & lambdaExitNodes = provisioning.GetLambdaExitNodes(*test.lambda);
    assert(lambdaExitNodes == expectedMemoryNodes);
  };

  StoreTest2 test;
  auto pointsToGraph = RunSteensgaard(test.module());

  /*
   * Act
   */
  auto provisioning = jlm::aa::LifetimeAwareMemoryNodeProvider::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *provisioning, *pointsToGraph);
}

static void
TestLoad1()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](
    const LoadTest1 & test,
    const jlm::aa::MemoryNodeProvisioning & provisioning,
    const jlm::aa::PointsToGraph & pointsToGraph)
  {
    auto & lambdaMemoryNode = pointsToGraph.GetLambdaNode(*test.lambda);
    auto & externalMemoryNode = pointsToGraph.GetExternalMemoryNode();

    jlm::HashSet<const jlm::aa::PointsToGraph::MemoryNode *> expectedMemoryNodes(
      {
        &lambdaMemoryNode,
        &externalMemoryNode
      });

    auto & lambdaEntryNodes = provisioning.GetLambdaEntryNodes(*test.lambda);
    assert(lambdaEntryNodes == expectedMemoryNodes);

    auto & lambdaExitNodes = provisioning.GetLambdaExitNodes(*test.lambda);
    assert(lambdaExitNodes == expectedMemoryNodes);
  };

  LoadTest1 test;
  auto pointsToGraph = RunSteensgaard(test.module());

  /*
   * Act
   */
  auto provisioning = jlm::aa::LifetimeAwareMemoryNodeProvider::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *provisioning, *pointsToGraph);
}

static void
TestLoad2()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](
    const LoadTest2 & test,
    const jlm::aa::MemoryNodeProvisioning & provisioning,
    const jlm::aa::PointsToGraph & pointsToGraph)
  {
    auto & lambdaMemoryNode = pointsToGraph.GetLambdaNode(*test.lambda);
    auto & externalMemoryNode = pointsToGraph.GetExternalMemoryNode();

    jlm::HashSet<const jlm::aa::PointsToGraph::MemoryNode *> expectedMemoryNodes(
      {
        &lambdaMemoryNode,
        &externalMemoryNode
      });

    auto & lambdaEntryNodes = provisioning.GetLambdaEntryNodes(*test.lambda);
    assert(lambdaEntryNodes == expectedMemoryNodes);

    auto & lambdaExitNodes = provisioning.GetLambdaExitNodes(*test.lambda);
    assert(lambdaExitNodes == expectedMemoryNodes);
  };

  LoadTest2 test;
  auto pointsToGraph = RunSteensgaard(test.module());

  /*
   * Act
   */
  auto provisioning = jlm::aa::LifetimeAwareMemoryNodeProvider::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *provisioning, *pointsToGraph);
}

static void
TestLoadFromUndef()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](
    const LoadFromUndefTest & test,
    const jlm::aa::MemoryNodeProvisioning & provisioning,
    const jlm::aa::PointsToGraph & pointsToGraph)
  {
    auto & lambdaMemoryNode = pointsToGraph.GetLambdaNode(test.Lambda());
    auto & externalMemoryNode = pointsToGraph.GetExternalMemoryNode();

    jlm::HashSet<const jlm::aa::PointsToGraph::MemoryNode *> expectedMemoryNodes(
      {
        &lambdaMemoryNode,
        &externalMemoryNode
      });

    auto & lambdaEntryNodes = provisioning.GetLambdaEntryNodes(test.Lambda());
    assert(lambdaEntryNodes == expectedMemoryNodes);

    auto & lambdaExitNodes = provisioning.GetLambdaExitNodes(test.Lambda());
    assert(lambdaExitNodes == expectedMemoryNodes);
  };

  LoadFromUndefTest test;
  auto pointsToGraph = RunSteensgaard(test.module());

  /*
   * Act
   */
  auto provisioning = jlm::aa::LifetimeAwareMemoryNodeProvider::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *provisioning, *pointsToGraph);
}

static int
TestLifetimeAwareMemoryNodeProvider()
{
  TestStore1();
  TestStore2();

  TestLoad1();
  TestLoad2();
  TestLoadFromUndef();

  return 0;
}

JLM_UNIT_TEST_REGISTER(
  "libjlm/opt/alias-analyses/TestLifetimeAwareMemoryNodeProvider",
  TestLifetimeAwareMemoryNodeProvider)