/*
 * @Date: 2025-01-27
 * @Description: 城市经纬度查找函数实现
 */
#include "city_data.h"
#include <cstring>

bool FindCityByName(const char* city_name, double& lat, double& lon) {
    for (size_t i = 0; i < city_count; i++) {
        if (strstr(city_coordinates[i].city, city_name) != nullptr ||
            strstr(city_name, city_coordinates[i].city) != nullptr) {
            lat = city_coordinates[i].lat;
            lon = city_coordinates[i].lon;
            return true;
        }
    }
    return false;
}

bool FindCityByAdmin(const char* admin_name, double& lat, double& lon) {
    for (size_t i = 0; i < city_count; i++) {
        if (strstr(city_coordinates[i].admin, admin_name) != nullptr ||
            strstr(admin_name, city_coordinates[i].admin) != nullptr) {
            lat = city_coordinates[i].lat;
            lon = city_coordinates[i].lon;
            return true;
        }
    }
    return false;
}
