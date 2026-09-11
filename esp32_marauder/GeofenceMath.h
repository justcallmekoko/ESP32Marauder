#pragma once
#include <math.h>

namespace GeofenceMath {
  inline double distanceMiles(double lat1, double lon1, double lat2, double lon2) {
    const double radians = 0.017453292519943295;
    const double dlat = (lat2 - lat1) * radians;
    const double dlon = (lon2 - lon1) * radians;
    const double a = sin(dlat / 2) * sin(dlat / 2) +
      cos(lat1 * radians) * cos(lat2 * radians) * sin(dlon / 2) * sin(dlon / 2);
    return 3958.7613 * (2 * atan2(sqrt(a), sqrt(1 - a)));
  }
}
