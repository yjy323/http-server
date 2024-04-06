#ifndef MULTIPLEXING_HPP
#define MULTIPLEXING_HPP

#include <vector>

#include "socket.hpp"

#pragma once

int Multiplexing(const Configuration& configuration);
int Multiplexing(const std::vector<Socket>& sockets);

#endif
