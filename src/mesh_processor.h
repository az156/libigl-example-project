#pragma once

#include <Eigen/Core>
#include <vector>

namespace Its {
	/*
	* @brief 网格状态
	* 可使用“|”组合多个状态，若同时存在重复顶点和孔洞，可令 status = Success_Duplicate | Success_Hole
	* 可使用“&”取出特定状态，若需检查是否存在孔洞，可判断 status & MeshStatus::Hole 为真
	*/
	enum MeshStatus {
		OK = 0,					// 网格完好
		Duplicate_Vertex = 2,	// 重复顶点
		Hole = 4,				// 孔洞
		Isolated_Parts = 8,		// 非连通
		Non_Manifold = 16,		// 非流形
		Normal_Error = 32,		// 法向错误
		Self_Intersect = 64,	// 自交
	};

	class __declspec(dllexport) MeshProcessor
	{
	private:
		// ------------------------------------- 数据 -------------------------------------
		Eigen::MatrixXd mV;

		Eigen::MatrixXi mF;

	public:
		MeshProcessor(const std::vector<double>& vertices, const std::vector<int>& faces);

		// 获取网格数据
		void getData(std::vector<double>& vertices, std::vector<int>& faces, std::vector<double>& normals);

		// ------------------------------------- 检测 -------------------------------------

		// 检测重复顶点
		bool CheckDuplicateVertex();

		// 检测孔洞
		bool CheckHole();

		// 检测非连通
		bool CheckIsolatedParts();

		// 检测非流形
		bool CheckNonManifold();

		// 检测法线错误
		bool CheckNormalError();

		// 检测自交
		bool CheckSelfIntersect();

		/*
		* @brief 自动检测
		* @return 返回 0 代表网格完好，其他值代表检测到一些问题（查阅 MeshStatus）
		*/
		int CheckAll();

		// ------------------------------------- 修复 -------------------------------------

		// 清除重复顶点
		bool RepairDuplicateVertex();

		// 修复孔洞
		bool RepairHole();

		// 修复非连通：清除与主体分离的网格
		bool RepairIsolatedParts();

		// 修复非流形
		bool RepairNonManifold();

		// 修复法线错误
		bool RepairNormalError();

		// 修复自交
		bool RepairSelfIntersect();

		/*
		* @brief 自动修复
		* @return 返回 0 代表网格完好，其他值代表修复了一些问题（查阅 MeshStatus）
		*/
		int RepairAll();

		// ------------------------------------- 布尔运算 -------------------------------------

		int BooleanUnion(Eigen::MatrixXd VB, Eigen::MatrixXi FB);

		// ------------------------------------- 其他 -------------------------------------
		
		// 分离不连通的网格
		bool SeparateMeshes(std::vector<Eigen::MatrixXd>& vertices, std::vector<Eigen::MatrixXi>& faces);
	};
}
