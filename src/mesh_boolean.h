#pragma once

#ifndef BOOLEAN_H
#define BOOLEAN_H

namespace Its {
	class __declspec(dllexport) MeshOperateHelper
	{
	public:
		// 计算模型布尔运算
		static int MeshBoolean();
	};
}

#endif // BOOLEAN_H