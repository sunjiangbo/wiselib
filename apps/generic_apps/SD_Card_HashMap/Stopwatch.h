/*
 * Stopwatch.h
 *
 *  Created on: Jan 20, 2013
 *      Author: maximilian
 */

#ifndef STOPWATCH_H_
#define STOPWATCH_H_

class Stopwatch
{
public:
	Stopwatch() : running(false), startTime(0), allTime(0)
	{

	}

	void startMeasurement()
	{
		if(running)
			return;
		else
		{
			running = true;
			startTime = micros();
		}
	}

	unsigned long stopMeasurement()
	{
		if(running)
		{
			unsigned long stoppedTime = micros() - startTime;
			allTime += stoppedTime;
			running = false;
			return stoppedTime;
		}
		else
			return 0;
	}

	unsigned long getAllTime()
	{
		return allTime;
	}

	void reset()
	{
		startTime = 0;
		running = false;
		allTime = 0;
	}

private:
		bool running;
		unsigned long startTime;
		unsigned long allTime;
};

Stopwatch readIOStopwatch;
Stopwatch writeIOStopwatch;
Stopwatch allStopwatch;


#endif /* STOPWATCH_H_ */
