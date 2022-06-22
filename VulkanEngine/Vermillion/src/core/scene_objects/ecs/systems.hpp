#pragma once

struct SystemBase
{
	std::set<Entity> entities;
};

struct Allocator : SystemBase
{

};
struct Deallocator : SystemBase
{

};