// Copyright (c) 2009, Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#ifdef _MSC_VER
# pragma once
#endif
#ifndef PROFILER_H
#define PROFILER_H
#include <Common/QuickFAST_Export.h>

/// enable or disable generation of profiler code.
#define PROFILER_ENABLE


// Note: boost's microsecond_clock was slow enough to skew the results
// hence the win32 GetTickCount stuff.
#ifdef _WIN32
# ifndef _WINBASE_ // if we didn't #include <windows.h>
// this is the ultimate expansion of the declaration for GetTickCount
// after the macros have been resolved on a 16 bit windows build
// With luck, it won't conflict with the windows declaration in case
// the included occur in the wrong order
typedef unsigned long DWORD;
__declspec(dllimport) DWORD GetTickCount(void);
# endif // _WINBASE_
# define PROFILER_GET_TIME ::GetTickCount()
# define PROFILER_TIME_TYPE DWORD
# define PROFILER_DIFF_MSEC(a, b) (a - b)
#else // _WIN32
# include <boost/date_time/posix_time/posix_time.hpp>
# define PROFILER_GET_TIME boost::posix_time::microsec_clock::universal_time()
# define PROFILER_TIME_TYPE boost::posix_time::ptime
# define PROFILER_DIFF_MSEC(a,b) ((a - b).total_milliseconds())
#endif // _WIN32

namespace QuickFAST{
  /// @brief Accumulate profiler statistics.
  ///
  /// A ProfileAccumulater is statically created for each Profile Point.
  /// The ProfileInstances created to do the actual timing store their results
  /// into the corresponding ProfileAccumulator.
  /// These accumulators link themselves together in a list starting at root_.
  /// Walking this list lets you find all profile points in the system.
  /// The static write(...) method writes a tab-delimited file of the statistics.
  /// Hint: try importing this file into a spreadsheet for analysis.
  class QuickFAST_Export ProfileAccumulator
  {
  public:
    /// @brief Create the ProfileAccumulator
    /// @param name identifies the profile point.
    /// @param file should be generated by the __FILE__ predefined macro.
    /// @param line should be generated by the __LINE__ predefined macro.
    ProfileAccumulator(const char * name, const char * file, size_t line);
    /// @brief write in machine-readable form (tab delimited columns)
    static void write(std::ostream & out);
    /// @brief write in somewhat human readable format
    static void print(std::ostream & out);

  private:
    friend class ProfileInstance;
    static ProfileAccumulator * root_;
    ProfileAccumulator * next_;
    const char * name_;
    const char * file_;
    size_t line_;
    size_t entries_;
    size_t exits_;
    size_t pauses_;
    size_t resumes_;
    double sum_;
    double sumOfSquares_;
    size_t recursions_;
    double recursiveSum_;
    double recursiveSumOfSquares_;
  };

  /// @brief an auto variable to measure the time in a section of code
  ///
  /// Measures time from creation to destruction (usually controlled by scope)
  /// and stores the results in a ProfileAccumulator
  class QuickFAST_Export ProfileInstance
  {
  public:

    /// @brief Construct and link to an accumulator
    /// @param accumulator to receive the measured results.
    ProfileInstance(ProfileAccumulator & accumulator)
      : accumulator_(accumulator)
      , start_(PROFILER_GET_TIME)
      , running_(true)
    {
      accumulator_.entries_ += 1;
    }

    /// @brief Stop timing and accumulate results.
    ~ProfileInstance()
    {
      stop();
      accumulator_.exits_ += 1;
    }

    /// @brief Stop timing -- may be resumable
    /// @returns false if it was already stopped.
    bool pause()
    {
      bool result = running_;
      stop();
      accumulator_.pauses_ += 1;
      return result;
    }

    /// @brief Resume timing after a pause
    /// @param pauseState is the return value from a pause
    void resume(bool pauseState)
    {
      accumulator_.resumes_ += 1;
      if(!running_ && pauseState)
      {
        start_ = PROFILER_GET_TIME;
        running_ = true;
      }
    }

  private:
    void stop()
    {
      if(running_)
      {
        PROFILER_TIME_TYPE now = PROFILER_GET_TIME;
        double lapse = double(PROFILER_DIFF_MSEC(now, start_));
        accumulator_.sum_ += lapse;
        accumulator_.sumOfSquares_ += lapse * lapse;
        assert(accumulator_.entries_ > accumulator_.exits_);
        if(accumulator_.entries_ != accumulator_.exits_ + 1)
        {
          accumulator_.recursions_ += 1;
          accumulator_.recursiveSum_ += lapse;
          accumulator_.recursiveSumOfSquares_ += lapse * lapse;
        }
        running_ = false;
      }
    }

  private:
    ProfileAccumulator & accumulator_;
    PROFILER_TIME_TYPE start_;
    bool running_;
  };
}

#ifdef PROFILER_ENABLE
/// Define the start point of a block of code to be profiled.
# define PROFILE_POINT(name)   \
    static ProfileAccumulator PROFILE_accumulator(name, __FILE__, __LINE__); \
    ProfileInstance PROFILE_instance(PROFILE_accumulator)

/// Pause or stop profiling
# define PROFILE_PAUSE \
    bool PROFILE_pauseState = PROFILE_instance.pause()

/// Resume after pause
# define PROFILE_RESUME \
    PROFILE_instance.pause(PROFILE_pauseState)

/// Define the start point of a block of code to be profiled.
/// Allows more than one profiler in the same scope.
# define NESTED_PROFILE_POINT(id, name) \
    static ProfileAccumulator PROFILE_accumulator##id(name, __FILE__, __LINE__); \
    ProfileInstance PROFILE_instance##id(PROFILE_accumulator##id)

/// Pause or stop profiling
# define NESTED_PROFILE_PAUSE(id) \
  bool PROFILE_pauseState##id = PROFILE_instance##id.pause()

/// Resume after pause
# define NESTED_PROFILE_RESUME(id) \
  PROFILE_instance##id.pause(PROFILE_pauseState##id)

#else // PROFILER_ENABLE

# define PROFILE_POINT(name)  void(0)
# define PROFILE_PAUSE  void(0)
# define NESTED_PROFILE_POINT(id, name)  void(0)
# define NESTED_PROFILE_PAUSE(id) void(0)
# define NESTED_PROFILE_RESUME(id) void(0)
#endif // PROFILER_ENABLE

#endif // PROFILER_H
