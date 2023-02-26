#pragma once
#include <sys/types.h>

/**
 * @brief Watches the maximum number of File Descriptors as FEXServer creates them.
 *
 * Attempts to raise the Linux process limit if we get close to the soft-limit.
 *
 * By default the FD soft-limit is usually around 1024, which FEXServer can exceed
 * if there are a large number of processes that FEXServer is tracking.
 *
 * Once FEXServer gets close to the limit, it will attempt raising the soft-limit by
 * 2x up until we get to the process hard-limit.
 *
 * On a default Ubuntu install the hard-limit is around a million FDs, but some distros
 * can be more strict than this.
 *
 * If FEXServer hits the soft-limit then instances of FEXInterpreter opening a socket to FEXServer
 * will hang.
 */
namespace FDCountWatch {
  void GetMaxFDs();
  void IncrementFDCountAndCheckLimits(size_t Num);
  void DecrementFDCountAndCheckLimits(size_t Num);
}
