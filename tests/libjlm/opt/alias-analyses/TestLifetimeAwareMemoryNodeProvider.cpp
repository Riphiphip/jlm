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
    JLM_UNREACHABLE("Not yet implemented!");
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

static int
TestLifetimeAwareMemoryNodeProvider()
{
  TestStore1();

  return 0;
}

JLM_UNIT_TEST_REGISTER(
  "libjlm/opt/alias-analyses/TestLifetimeAwareMemoryNodeProvider",
  TestLifetimeAwareMemoryNodeProvider)