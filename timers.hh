#pragma once

// Stores the CPU clock times for different parts of the algorithm
struct TIMERS {
	long timetot;
	long timesim;
	long timeboot;
};

extern TIMERS timers; // Stores the CPU clock times for different parts of the algorithm