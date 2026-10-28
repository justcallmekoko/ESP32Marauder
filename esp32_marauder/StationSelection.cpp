#include "StationSelection.h"

int associatedApToSelect(bool station_selected, uint16_t ap_index, size_t ap_count) {
  return station_selected && ap_index < ap_count ? static_cast<int>(ap_index) : -1;
}
