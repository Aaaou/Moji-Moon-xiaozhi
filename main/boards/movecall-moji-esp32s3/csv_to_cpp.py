#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将城市经纬度CSV文件转换为C++数组格式
用于ESP32固件中的城市坐标查询
"""

import csv
import sys
import os

def convert_csv_to_cpp(csv_file_path, output_file_path):
    """
    转换CSV文件为C++数组
    """
    print(f"正在读取CSV文件: {csv_file_path}")
    
    cities_data = []
    
    try:
        with open(csv_file_path, 'r', encoding='utf-8') as file:
            reader = csv.reader(file)
            
            # 读取表头
            headers = next(reader)
            print(f"检测到的列: {headers}")
            print(f"列数: {len(headers)}")
            
            # 显示前几行数据来帮助调试
            print("\n前3行数据示例:")
            sample_rows = []
            for i, row in enumerate(reader):
                if i >= 3:
                    break
                sample_rows.append(row)
                print(f"第{i+2}行: {row}")
            
            # 重新定位到文件开头，跳过表头
            file.seek(0)
            next(csv.reader(file))
            
            # 根据表头进行列映射
            if len(headers) >= 7:
                # ['', '城市ID', '行政归属', '城市简称', '拼音', 'lat', 'lon']
                city_id_col = 1      # 城市ID在第2列
                admin_col = 2        # 行政归属在第3列
                city_name_col = 3    # 城市简称在第4列
                pinyin_col = 4       # 拼音在第5列
                lat_col = 5          # 纬度在第6列
                lon_col = 6          # 经度在第7列
            else:
                print("错误: CSV格式不符合预期，应包含至少7列")
                return False
            
            print(f"列映射: 城市ID={city_id_col}, 行政归属={admin_col}, 城市名称={city_name_col}, 拼音={pinyin_col}, 纬度={lat_col}, 经度={lon_col}")
            
            # 读取数据
            for row_num, row in enumerate(reader, start=2):
                try:
                    if len(row) < 6:
                        continue
                        
                    city_id = row[city_id_col].strip()
                    admin = row[admin_col].strip() 
                    city_name = row[city_name_col].strip()
                    pinyin = row[pinyin_col].strip()
                    # 从示例看，经纬度在拼音列后面
                    lat_str = row[lat_col].strip() if len(row) > lat_col else ""
                    lon_str = row[lon_col].strip() if len(row) > lon_col else ""
                    
                    # 检查经纬度是否为有效数字
                    try:
                        # 如果经纬度列为空，尝试从拼音和城市ID列获取
                        if not lat_str and not lon_str:
                            # 示例: ['0', 'WX4FBXXFKE4F', '北京市', '北京/北京', 'Beijing', '39.9', '116.4']
                            lat_str = row[pinyin_col + 1].strip() if len(row) > pinyin_col + 1 else ""
                            lon_str = row[pinyin_col + 2].strip() if len(row) > pinyin_col + 2 else ""
                        
                        lat = float(lat_str)
                        lon = float(lon_str)
                    except (ValueError, IndexError):
                        # 如果是文本或索引错误，跳过这行
                        if row_num <= 10:  # 只显示前10个错误
                            print(f"跳过第{row_num}行: 经纬度解析失败 (lat='{lat_str}', lon='{lon_str}'), 行数据: {row}")
                        continue
                    
                    # 检查经纬度范围是否合理（中国大陆范围大致检查）
                    if not (-90 <= lat <= 90 and -180 <= lon <= 180):
                        continue
                    
                    if city_name and not (lat == 0 and lon == 0):
                        cities_data.append({
                            'id': city_id,
                            'admin': admin,
                            'city': city_name,
                            'pinyin': pinyin,
                            'lat': lat,
                            'lon': lon
                        })
                        
                except (IndexError) as e:
                    if row_num <= 10:  # 只显示前10个错误
                        print(f"警告: 第{row_num}行数据格式错误: {e}")
                    continue
        
        print(f"成功读取 {len(cities_data)} 个城市数据")
        
        # 生成C++代码
        with open(output_file_path, 'w', encoding='utf-8') as output:
            output.write("""/*
 * @Date: 2025-01-27
 * @Description: 城市经纬度数据，从CSV文件自动生成
 * @Generated: 自动生成，请勿手动编辑
 */
#pragma once

struct CityData {
    const char* id;       // 城市ID
    const char* admin;    // 行政归属
    const char* city;     // 城市名称
    const char* pinyin;   // 拼音
    double lat;           // 纬度
    double lon;           // 经度
};

static const CityData city_coordinates[] = {
""")
            
            for city in cities_data:
                output.write(f'    {{"{city["id"]}", "{city["admin"]}", "{city["city"]}", "{city["pinyin"]}", {city["lat"]:.6f}, {city["lon"]:.6f}}},\n')
            
            output.write(f"""
}};

static const size_t city_count = {len(cities_data)};

// 城市查找函数声明
bool FindCityByName(const char* city_name, double& lat, double& lon);
bool FindCityByAdmin(const char* admin_name, double& lat, double& lon);
""")
        
        # 生成查找函数实现
        impl_file = output_file_path.replace('.h', '_impl.cc')
        with open(impl_file, 'w', encoding='utf-8') as impl:
            impl.write(f'''/*
 * @Date: 2025-01-27
 * @Description: 城市经纬度查找函数实现
 */
#include "{os.path.basename(output_file_path)}"
#include <cstring>

bool FindCityByName(const char* city_name, double& lat, double& lon) {{
    for (size_t i = 0; i < city_count; i++) {{
        if (strstr(city_coordinates[i].city, city_name) != nullptr ||
            strstr(city_name, city_coordinates[i].city) != nullptr) {{
            lat = city_coordinates[i].lat;
            lon = city_coordinates[i].lon;
            return true;
        }}
    }}
    return false;
}}

bool FindCityByAdmin(const char* admin_name, double& lat, double& lon) {{
    for (size_t i = 0; i < city_count; i++) {{
        if (strstr(city_coordinates[i].admin, admin_name) != nullptr ||
            strstr(admin_name, city_coordinates[i].admin) != nullptr) {{
            lat = city_coordinates[i].lat;
            lon = city_coordinates[i].lon;
            return true;
        }}
    }}
    return false;
}}
''')
        
        print(f"C++头文件已生成: {output_file_path}")
        print(f"C++实现文件已生成: {impl_file}")
        print(f"包含 {len(cities_data)} 个城市的数据")
        
        # 生成统计信息
        provinces = set()
        for city in cities_data:
            provinces.add(city['admin'])
        
        print(f"涵盖 {len(provinces)} 个省/市/自治区")
        print("前10个城市示例:")
        for i, city in enumerate(cities_data[:10]):
            print(f"  {city['city']} ({city['admin']}) - {city['lat']:.4f}, {city['lon']:.4f}")
        
        return True
        
    except FileNotFoundError:
        print(f"错误: 找不到文件 {csv_file_path}")
        return False
    except Exception as e:
        print(f"错误: {e}")
        return False

def main():
    if len(sys.argv) != 3:
        print("用法: python csv_to_cpp.py <输入CSV文件> <输出头文件>")
        print('示例: python csv_to_cpp.py "D:\\Download\\V3城市对应经纬度 20230505.csv" city_data.h')
        return
    
    csv_file = sys.argv[1]
    cpp_file = sys.argv[2]
    
    if convert_csv_to_cpp(csv_file, cpp_file):
        print("转换完成!")
        print("使用方法:")
        print("1. 将生成的 .h 和 _impl.cc 文件包含到项目中")
        print("2. 调用 FindCityByName() 或 FindCityByAdmin() 查找城市坐标")
    else:
        print("转换失败!")

if __name__ == "__main__":
    main()
