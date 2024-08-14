#include "util/validation.hpp"

using namespace balatrogether;

bool validation::string(json& data)
{
  return data.is_string();
}

bool validation::string(json& data, size_t maxLength)
{
  return validation::string(data, 0, maxLength);
}

bool validation::string(json& data, size_t minLength, size_t maxLength)
{
  if (!validation::string(data)) return false;
  balatrogether::string str = data.get<balatrogether::string>();
  if (str.size() < minLength || str.size() > maxLength) return false;
  return true;
}

bool validation::integer(json& data, int min, int max)
{
  if (!data.is_number_integer()) return false;
  int num = data.get<int>();
  if (num < min || num > max) return false;
  return true;
}

bool validation::decimal(json &data, double min, double max)
{
  if (!data.is_number_float()) return false;
  double num = data.get<double>();
  if (num < min || num > max) return false;
  return true;
}

bool validation::boolean(json &data)
{
  return data.is_boolean();
}

bool validation::steamid(json& data)
{
  if (!validation::string(data, 32)) return false;
  steamid_t str = data.get<steamid_t>();
  uint64_t num = strtoull(str.c_str(), nullptr, 10);
  if (std::to_string(num) != str) return false;
  return true;
}
