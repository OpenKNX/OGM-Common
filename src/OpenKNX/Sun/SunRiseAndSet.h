// Implementation of Sun Rise and Set Times based on SUNRISET.C by Paul Schlyter 1989-2013.
// Original C code available at https://www.stjarnhimlen.se/comp/sunriset.c (copy in sunriset.c.txt)
// was modified for C++ and OpenKNX.
// For detailed explanation/documentation see https://www.stjarnhimlen.se/comp/riset.html [accessed 2025-01-30]

#pragma once

namespace OpenKNX
{
    namespace Sun
    {
        class SunRiseAndSet
        {
          private:
            static void sunPos(double d, double *lon, double *r);
            static void sunRadDec(double d, double *RA, double *dec, double *r);
            static double revolution(double x);
            static double rev180(double x);
            static double GMST0(double d);

          public:
            /* sunset and sunrise calulation */
            static int sunRiseSet(int year, int month, int day, double lon, double lat,
                                  double altit, int upper_limb, double *trise, double *tset);
        };
    } // namespace Sun
} // namespace OpenKNX