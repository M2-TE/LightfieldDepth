#pragma once

#define VMI_TIMER_BEGIN() auto begin = std::chrono::high_resolution_clock::now();
#define VMI_TIMER_END() VMI_LOG(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-begin).count() << "ms")
#define VMI_TIME_EXEC(func) VMI_TIMER_BEGIN(); func; VMI_TIMER_END();