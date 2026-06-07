TEMPLATE = subdirs
CONFIG  += ordered

# Build & run convention matches test/matrixrain/matrixrain_test.pro pattern.
# Each subdir produces its own QTEST_MAIN binary.
#
# Run all tests:
#   qmake hardware_test.pro && make -j && \
#       (cd keeper_test && ./test_activitySessionKeeper) && \
#       (cd suppressor_test && ./test_phantomWakeSuppressor)
#
# CI integration: see .github/workflows/test.yml `hardware-tests` job.

SUBDIRS  = keeper_test suppressor_test entityController_test

keeper_test.file            = keeper_test/keeper_test.pro
suppressor_test.file        = suppressor_test/suppressor_test.pro
entityController_test.file  = entityController_test/entityController_test.pro
