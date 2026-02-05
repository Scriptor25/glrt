#pragma once

#include <filesystem>

struct model_t;

void read_obj(const std::filesystem::path &path, model_t &model);
void read_mtl(const std::filesystem::path &path, model_t &model);
