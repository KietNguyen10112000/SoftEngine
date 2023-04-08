#pragma once

#define TASK_SYSTEM_UNPACK_PARAM_1(Type, p, p1)				\
auto param = (Type*)p;										\
auto p1 = param->p1;


#define TASK_SYSTEM_UNPACK_PARAM_2(Type, p, p1, p2)			\
TASK_SYSTEM_UNPACK_PARAM_1(Type, p, p1)						\
auto p2 = param->p2;


#define TASK_SYSTEM_UNPACK_PARAM_3(Type, p, p1, p2, p3)		\
TASK_SYSTEM_UNPACK_PARAM_2(Type, p, p1, p2)					\
auto p3 = param->p3;


#define TASK_SYSTEM_UNPACK_PARAM_4(Type, p, p1, p2, p3, p4)		\
TASK_SYSTEM_UNPACK_PARAM_3(Type, p, p1, p2, p3)					\
auto p4 = param->p4;


#define TASK_SYSTEM_UNPACK_PARAM_5(Type, p, p1, p2, p3, p4, p5)		\
TASK_SYSTEM_UNPACK_PARAM_4(Type, p, p1, p2, p3, p4)					\
auto p5 = param->p5;


//#define TASK_SYSTEM_UNPACK_PARAM_6(Type, p, p1, p2)		\
//auto param = (Type*)p;									\
//auto p1 = param->p1;									\
//auto p2 = param->p2;
//
//
//#define TASK_SYSTEM_UNPACK_PARAM_7(Type, p, p1, p2)		\
//auto param = (Type*)p;									\
//auto p1 = param->p1;									\
//auto p2 = param->p2;
//
//
//#define TASK_SYSTEM_UNPACK_PARAM_8(Type, p, p1, p2)		\
//auto param = (Type*)p;									\
//auto p1 = param->p1;									\
//auto p2 = param->p2;