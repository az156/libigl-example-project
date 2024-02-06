#include "mesh_processor.h"

// igl
#include <igl/edges.h>							// 获取边的拓扑信息
#include <igl/edge_topology.h>					// 获取边的拓扑信息
#include <igl/fast_find_self_intersections.h>	// 获取自交信息
#include <igl/per_face_normals.h>				// 获取三角形法线
#include <igl/triangle_triangle_adjacency.h>	// 获取三角形拓扑关系
#include <igl/boundary_loop.h>					// 查找边界环
#include <igl/remove_duplicate_vertices.h>		// 去除重复顶点
#include <igl/remove_unreferenced.h>			// 去除未被引用的顶点
#include <igl/facet_components.h>				// 检测连通分量
#include <igl/copyleft/cgal/intersect_other.h>	// 三角形相交检测
#include <igl/bounding_box.h>					// 计算包围盒

#include <igl/copyleft/cgal/mesh_boolean.h>		// 布尔运算

// debug
//#include <iostream>

// ------------------------------------- 数据 -------------------------------------

Its::MeshProcessor::MeshProcessor(const std::vector<double>& vertices, const std::vector<int>& faces)
{
	if (vertices.empty() || faces.empty()) {
		throw std::invalid_argument("Input is empty.");
	}
	// 确保输入向量的长度是3的倍数
	if (vertices.size() % 3 != 0 || faces.size() % 3 != 0) {
		throw std::invalid_argument("Input size is not a multiple of 3.");
	}
	// 使用Eigen::Map创建Eigen矩阵的视图
	Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>> matrixV(vertices.data(), vertices.size() / 3, 3);
	Eigen::Map<const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor>> matrixF(faces.data(), faces.size() / 3, 3);

	mV = matrixV;
	mF = matrixF;
}

void Its::MeshProcessor::getData(std::vector<double>& vertices, std::vector<int>& faces, std::vector<double>& normals)
{
	Eigen::MatrixXd N;
	igl::per_face_normals(mV, mF, N);

	// 确保矩阵以行优先方式存储
	Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixV = mV;
	Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixF = mF;
	Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrixN = N;

	// 使用Eigen::Map创建一个向量视图
	Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> mapV(matrixV.data(), matrixV.rows(), matrixV.cols());
	Eigen::Map<Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> mapF(matrixF.data(), matrixF.rows(), matrixF.cols());
	Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> mapN(matrixN.data(), matrixN.rows(), matrixN.cols());

	// 从map中提取std::vector
	vertices = std::vector<double>(mapV.data(), mapV.data() + mapV.size());
	faces = std::vector<int>(mapF.data(), mapF.data() + mapF.size());
	normals = std::vector<double>(mapN.data(), mapN.data() + mapN.size());
}

// ------------------------------------- 检测 -------------------------------------

bool Its::MeshProcessor::CheckDuplicateVertex()
{
	Eigen::MatrixXd NV;
	Eigen::MatrixXi NF;
	Eigen::VectorXi I, J;
	igl::remove_duplicate_vertices(mV, mF, 0.00001, NV, I, J, NF); // TODO：epsilon 表示容差，后续可调整

	return NV.rows() != mV.rows();
}

bool Its::MeshProcessor::CheckHole()
{
	std::vector<std::vector<int>> boundary_loops;
	igl::boundary_loop(mF, boundary_loops);

	return !boundary_loops.empty();
}

bool Its::MeshProcessor::CheckIsolatedParts()
{
	// 计算连通分量
	Eigen::VectorXi C;
	igl::facet_components(mF, C);

	return C.maxCoeff() > 0;
}

bool Its::MeshProcessor::CheckNonManifold()
{
	Eigen::MatrixXi E, uE;
	Eigen::VectorXi EMAP;
	Eigen::MatrixXi EF, EI;

	/*
	* 计算唯一边映射，F 是输入，其他是输出，其中 EMAP 是有向边到唯一无向边的映射，EF、EI共同表达唯一无向边到有向边的映射
	* E：所有有向边
	* uE：唯一无向边
	* EMAP：有向边到唯一无向边的映射
	* EF：存储了每个唯一边在 EI 中的起始索引，每个唯一边对应的有向边数量由 EF 中的下一个值与当前值的差决定
	* EI：存储了 uE 到 E 的映射，即每个唯一边对应的所有有向边在 E 中的索引
	*/
	igl::unique_edge_map(mF, E, uE, EMAP, EF, EI);

	// 查找非交叠边（每条边所属面的数量大于2）
	std::vector<int> non_manifold_edges;
	for (int i = 1; i < EF.rows(); ++i) {
		int offset = EF(i, 0) - EF(i - 1, 0);
		if (offset > 2) {
			return true;
		}
	}

	return false;
}

bool Its::MeshProcessor::CheckNormalError()
{
	// 计算每个面的法线
	Eigen::MatrixXd N;
	igl::per_face_normals(mV, mF, N);

	// 计算每个三角形与其邻接三角形的关系
	Eigen::MatrixXi TT, TTi;
	igl::triangle_triangle_adjacency(mF, TT, TTi);

	// 检测法线错误的孤立三角形
	for (int i = 0; i < mF.rows(); ++i) {
		int f1 = i;
		int inconsistent_normal_count = 0;
		for (int j = 0; j < 3; ++j) {
			// 检查相邻法向是否一致
			int f2 = TT(i, j);
			if (f2 != -1 && N.row(f1).dot(N.row(f2)) < 0.0) {
				++inconsistent_normal_count;
			}
		}
		if (inconsistent_normal_count == 3) {
			// 该三角形与相邻的三个三角形法向都不一致
			return true;
		}
	}

	return false;
}

bool Its::MeshProcessor::CheckSelfIntersect()
{
	Eigen::MatrixXi intersect;
	return igl::fast_find_self_intersections(mV, mF, intersect); // TODO 待优化
}

int Its::MeshProcessor::CheckAll()
{
	int result = 0;

	if (CheckDuplicateVertex()) {
		result |= Its::MeshStatus::Duplicate_Vertex;	// 检测重复顶点
	}

	//if (CheckIsolatedParts()) {
	//	result |= Its::MeshStatus::Isolated_Parts;		// 检测非连通
	//}

	if (CheckNonManifold()) {
		result |= Its::MeshStatus::Non_Manifold;		// 检测非流形
	}

	//if (CheckSelfIntersect()) {
	//	result |= Its::MeshStatus::Self_Intersect;		// 检测自交
	//}

	if (CheckNormalError()) {
		result |= Its::MeshStatus::Normal_Error;		// 检测法线错误
	}

	if (CheckHole()) {
		result |= Its::MeshStatus::Hole;				// 检测孔洞
	}

	return result;
}

// ------------------------------------- 检测 -------------------------------------

bool Its::MeshProcessor::RepairDuplicateVertex()
{
	Eigen::MatrixXd NV;
	Eigen::MatrixXi NF;
	Eigen::VectorXi I, J;
	igl::remove_duplicate_vertices(mV, mF, 0, NV, I, J, NF); // TODO：epsilon 表示容差，后续可调整

	if (mV.rows() != NV.rows()) {
		mV = NV;
		mF = NF;
		return true;
	}
	return false;
}

bool Its::MeshProcessor::RepairHole()
{
	// 搜环
	std::vector<std::vector<int>> boundary_loops;
	igl::boundary_loop(mF, boundary_loops);

	if (boundary_loops.empty()) {
		return false;
	}

	// 补洞
	std::list<Eigen::VectorXi> new_face_list;
	for (const auto& loop : boundary_loops) {
		if (loop.size() > 2) {
			// 使用第一个顶点作为公共顶点，生成新的三角形
			for (int i = 1; i < loop.size() - 1; ++i) {
				Eigen::VectorXi vec(3);
				vec << loop[0], loop[i + 1], loop[i]; // 新三角形有向边方向应与loop相反
				new_face_list.emplace_back(vec);
			}
		}
	}

	// 调整矩阵以容纳旧数据和新数据
	int currentRow = mF.rows();
	mF.conservativeResize(mF.rows() + new_face_list.size(), Eigen::NoChange);

	// 添加新的三角形数据
	for (const auto& triangle : new_face_list) {
		mF.row(currentRow++) = triangle;
	}

	return true;
}

bool Its::MeshProcessor::RepairIsolatedParts()
{
	std::vector<Eigen::MatrixXd> vertices;
	std::vector<Eigen::MatrixXi> faces;

	if (!SeparateMeshes(vertices, faces)) {
		return false;
	}
	// 计算每个连通网格的体积
	std::vector<double> volume_list;
	for (int i = 0; i < vertices.size(); ++i) {
		Eigen::MatrixXd BV, BF;
		igl::bounding_box(vertices[i], BV, BF);

		// 计算每个轴向的长度
		double lengthX = BV.col(0).maxCoeff() - BV.col(0).minCoeff();
		double lengthY = BV.col(1).maxCoeff() - BV.col(1).minCoeff();
		double lengthZ = BV.col(2).maxCoeff() - BV.col(2).minCoeff();

		volume_list.emplace_back(lengthX * lengthY * lengthZ);
	}
	// 找到体积最大的模型
	int max_index = 0;
	for (int i = 1; i < volume_list.size(); ++i) {
		if (volume_list[i] > volume_list[max_index]) {
			max_index = i;
		}
	}
	mV = vertices[max_index];
	mF = faces[max_index];

	return true;
}

bool Its::MeshProcessor::RepairNonManifold()
{
	Eigen::MatrixXi E, uE;
	Eigen::VectorXi EMAP;
	Eigen::MatrixXi EF, EI;

	/*
	* 计算唯一边映射，F 是输入，其他是输出，其中 EMAP 是有向边到唯一无向边的映射，EF、EI共同表达唯一无向边到有向边的映射
	* E：所有有向边
	* uE：唯一无向边
	* EMAP：有向边到唯一无向边的映射
	* EF：存储了每个唯一边在 EI 中的起始索引，每个唯一边对应的有向边数量由 EF 中的下一个值与当前值的差决定
	* EI：存储了 uE 到 E 的映射，即每个唯一边对应的所有有向边在 E 中的索引
	*/
	igl::unique_edge_map(mF, E, uE, EMAP, EF, EI);

	// 查找非交叠边（每条边所属面的数量大于2） 
	std::vector<int> non_manifold_edges;
	for (int i = 0; i < EF.rows() - 1; ++i) {
		int startIndex = EF(i, 0);
		int nextIndex = EF(i + 1, 0);
		if (nextIndex - startIndex > 2) {
			for (int j = startIndex; j < nextIndex; ++j) {
				int edgeIndex = EI(j);
				non_manifold_edges.push_back(E(edgeIndex));
			}
		}
	}

	if (non_manifold_edges.empty()) {
		return false;
	}

	// 标记包含非交叠边的面
	int face_count = mF.rows();
	Eigen::VectorXi face_flags = Eigen::VectorXi::Ones(mF.rows());  // 初始化为保留所有面

	for (int index : non_manifold_edges) {
		// 根据 unique_edge_map() 函数中构造 E 的方法 oriented_facets() 可知
		// 边索引对 F.row() 取余既是对应的面索引
		int face_index = index % face_count;
		face_flags(face_index) = 0;  // 标记为删除
	}

	// 构建新的面矩阵
	std::vector<Eigen::RowVector3i> correct_faces;
	for (int i = 0; i < mF.rows(); ++i) {
		if (face_flags(i) == 1) {
			correct_faces.push_back(mF.row(i));
		}
	}

	// 转换为Eigen矩阵
	mF.resize(correct_faces.size(), 3);
	for (size_t i = 0; i < correct_faces.size(); ++i) {
		mF.row(i) = correct_faces[i];
	}

	// 删除未引用的顶点并更新网格
	Eigen::VectorXi I;
	igl::remove_unreferenced(mV, mF, mV, mF, I);

	return true;
}

bool Its::MeshProcessor::RepairNormalError()
{
	// 计算每个面的法线
	Eigen::MatrixXd N;
	igl::per_face_normals(mV, mF, N);

	// 计算每个三角形与其邻接三角形的关系
	Eigen::MatrixXi TT, TTi;
	igl::triangle_triangle_adjacency(mF, TT, TTi);

	// 检测法线错误的孤立三角形
	bool inconsistent_normal_flag = false; // 存在法线不一致的三角形
	for (int i = 0; i < mF.rows(); ++i) {
		int f1 = i;
		int inconsistent_normal_count = 0;
		for (int j = 0; j < 3; ++j) {
			// 检查相邻法向是否一致
			int f2 = TT(i, j);
			if (f2 != -1 && N.row(f1).dot(N.row(f2)) < 0.0) {
				++inconsistent_normal_count;
			}
		}
		if (inconsistent_normal_count == 3) {
			// 该三角形与相邻的三个三角形法向都不一致，需要置反
			mF.row(f1).reverseInPlace();
			inconsistent_normal_flag = true;
		}
	}

	return inconsistent_normal_flag;
}

bool Its::MeshProcessor::RepairSelfIntersect()
{
	// 检测自相交
	Eigen::MatrixXi intersect;
	if (!igl::fast_find_self_intersections(mV, mF, intersect)) {
		return false;
	}

	// 收集所有不自交面的索引
	std::vector<Eigen::VectorXi> new_faces;
	for (int i = 0; i < mF.rows(); ++i) {
		if (intersect(i, 0) == 0) {
			Eigen::VectorXi vec(3);
			vec << mF(i, 0), mF(i, 1), mF(i, 2);
			new_faces.emplace_back(vec);
		}
	}

	// 调整尺寸以包含新的面
	Eigen::MatrixXi temp_face(new_faces.size(), 3);
	for (int i = 0; i < new_faces.size(); ++i) {
		temp_face.row(i) << new_faces[i](0), new_faces[i](1), new_faces[i](2);
	}

	RepairHole();

	Eigen::VectorXi I, J;
	igl::remove_unreferenced(mV, mF, mV, mF, I, J);

	return true; // TODO
}


int Its::MeshProcessor::RepairAll()
{
	int result = 0;

	if (RepairDuplicateVertex()) {
		result |= Its::MeshStatus::Duplicate_Vertex;	// 修复重复顶点
	}

	if (RepairIsolatedParts()) {
		result |= Its::MeshStatus::Isolated_Parts;		// 修复非连通
	}

	//if (RepairNonManifold()) {
	//	result |= Its::MeshStatus::Non_Manifold;		// 修复非流形
	//}

	//if (RepairSelfIntersect()) {
	//	result |= Its::MeshStatus::Self_Intersect;		// 修复自交
	//}

	if (RepairHole()) {
		result |= Its::MeshStatus::Hole;				// 修复孔洞
	}

	if (RepairNormalError()) {
		result |= Its::MeshStatus::Normal_Error;		// 修复法线错误
	}

	return result;
}

int Its::MeshProcessor::BooleanUnion(Eigen::MatrixXd VB, Eigen::MatrixXi FB)
{
	// 执行布尔运算
	Eigen::MatrixXd VU;
	Eigen::MatrixXi FU;
	igl::copyleft::cgal::mesh_boolean(mV, mF, VB, FB, igl::MeshBooleanType::MESH_BOOLEAN_TYPE_MINUS, VU, FU);

	return 0;
}

// ------------------------------------- 其他 -------------------------------------

bool Its::MeshProcessor::SeparateMeshes(std::vector<Eigen::MatrixXd>& vertices, std::vector<Eigen::MatrixXi>& faces)
{
	// 计算连通分量
	Eigen::VectorXi C;
	igl::facet_components(mF, C);

	// 确定连通分量的数量
	int num_components = C.maxCoeff() + 1;

	// 如果网格是连通的（只有一个连通分量），返回 0
	if (num_components == 1) {
		return false;
	}

	// 为每个连通分量创建一个面矩阵
	std::vector<Eigen::MatrixXi> component_faces(num_components);
	for (int i = 0; i < mF.rows(); ++i) {
		// 获取当前面所在的连通分量索引
		int comp = C(i);
		// 将当前面添加到对应连通分量的面矩阵中
		component_faces[comp].conservativeResize(component_faces[comp].rows() + 1, 3);
		component_faces[comp].row(component_faces[comp].rows() - 1) = mF.row(i);
	}

	for (int i = 0; i < component_faces.size(); ++i) {
		// temp vertices and faces
		Eigen::MatrixXd temp_vertices;
		Eigen::MatrixXi temp_faces;
		Eigen::VectorXi I, J;
		igl::remove_unreferenced(mV, component_faces[i], temp_vertices, temp_faces, I, J);

		vertices.emplace_back(temp_vertices);
		faces.emplace_back(temp_faces);
	}

	return true;
}
