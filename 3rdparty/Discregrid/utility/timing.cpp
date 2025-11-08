#include "timing.hpp"

using namespace Discregrid;

std::unordered_map<int, AverageTime> Timing::m_averageTimes;
std::stack<TimingHelper> Timing::m_timingStack;
bool Timing::m_dontPrintTimes = false;
unsigned int Timing::m_startCounter = 0;
unsigned int Timing::m_stopCounter = 0;
unsigned int Timing::m_id_counter = 0;
