#pragma once

#define ROF_CTOR_DELETE(name)			\
name() = delete;

#define ROF_COPY_DELETE(name)			\
name(const name& other) = delete;				\
name& operator=(const name& other) = delete;

#define ROF_MOVE_DELETE(name)			\
name(name&& other) = delete;					\
name& operator=(name&& other) = delete;

#define ROF_COPY_MOVE_DELETE(name)		\
ROF_COPY_DELETE(name)					\
ROF_MOVE_DELETE(name)

#define ROF_ALL_DELETE(name)			\
ROF_CTOR_DELETE(name)					\
ROF_COPY_DELETE(name)					\
ROF_MOVE_DELETE(name)