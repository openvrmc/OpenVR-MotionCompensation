#pragma once

#include <openvr_driver.h>

#include <boost/timer/timer.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/system_clocks.hpp>

#include <mutex>

#define MAX_DEBUG_ENTRIES 50000
#define MAX_DEBUG_VECTORS 24
#define MAX_DEBUG_QUATERNIONS 8

namespace vrmotioncompensation
{
	namespace driver
	{
		class Debugger
		{

		private:

			struct DebugData
			{
				std::string Name = "";
				bool InUse = false;
			};

			struct DebugDataV3 : DebugData 
			{
				vr::HmdVector3d_t Data[MAX_DEBUG_ENTRIES];
			};

			struct DebugDataQ4 : DebugData
			{
				vr::HmdQuaternion_t Data[MAX_DEBUG_ENTRIES];
			};

			template< class Clock > class timer
			{
				typename Clock::time_point tstart;
			public:
				timer()
				{
				}
				typename Clock::duration elapsed() const
				{
					return Clock::now() - tstart;
				}
				double seconds() const
				{
					return elapsed().count() * ((double)Clock::period::num / Clock::period::den);
				}
				void start()
				{
					tstart = Clock::now();
				}
			};

		public:		
			Debugger();
			~Debugger();

			void Start();

			void Stop();

			bool IsRunning();

			void CountUp();

			void AddDebugData(vr::HmdVector3d_t Data, int ID);

			void AddDebugData(vr::HmdQuaternion_t Data, int ID);

			void AddDebugData(const double Data[3], int ID);

			void SetDebugNameV3(std::string Name, int ID);
			void SetDebugNameQ4(std::string Name, int ID);

			void WriteFile();

			void gotRef();
			void gotHmd();
			bool hasRef();
			bool hasHmd();

		private:
			int DebugCounter = 0;
			DebugDataV3 DebugDataV3[MAX_DEBUG_VECTORS];
			DebugDataQ4 DebugDataQ4[MAX_DEBUG_QUATERNIONS];

			timer<boost::chrono::high_resolution_clock> DebugTimer;
			double DebugTiming[MAX_DEBUG_ENTRIES];

			bool DebuggerRunning = false;
			bool Ref = false;
			bool Hmd = false;
			bool WroteToFile = false;

			std::recursive_mutex _mut;
		};
	}
}