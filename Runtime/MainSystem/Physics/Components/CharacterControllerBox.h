#pragma once

#include "CharacterController.h"

NAMESPACE_BEGIN

struct CharacterControllerBoxDesc
{
	Box box;
};

class CharacterControllerBox : public CharacterController
{
public:
	CharacterControllerBox(const CharacterControllerBoxDesc& desc);

};

NAMESPACE_END