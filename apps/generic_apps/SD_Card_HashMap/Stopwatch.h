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
			startTime = millis();
		}
	}

	unsigned long int stopMeasurement()
	{
		unsigned long int stoppedTime = millis() - startTime;
		if(running)
		{
			allTime += stoppedTime;
			running = false;
			return stoppedTime;
		}
		else
			return 0;
	}

	unsigned long int getAllTime()
	{
		return allTime;
	}

	void reset()
	{
		startTime = 0;
		running = false;
		allTime = 0;
	}

	unsigned long int millis()
	{
		return 0;
	}

private:
		bool running;
		unsigned long int startTime;
		unsigned long int allTime;
};

Stopwatch IOStopwatch;
//Stopwatch writeIOStopwatch;
Stopwatch allStopwatch;


#endif /* STOPWATCH_H_ */
