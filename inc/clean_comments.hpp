#ifndef CLEAN_COMMENTS_HPP
#define CLEAN_COMMENTS_HPP

#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm> // for std::remove

// #include "clean_comments.hpp"

std::string clean_comments(std::ifstream &fs, std::string output_file, bool write_output_file = false);

#endif