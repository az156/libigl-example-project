#include "mesh_boolean.h"

// 布尔运算
#include <igl/copyleft/cgal/mesh_boolean.h>

int Its::MeshOperateHelper::MeshBoolean()
{
	// 创建平面
	Eigen::MatrixXd VA = (Eigen::MatrixXd(8, 3) <<
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 0.0,
		1.0, 1.0, 1.0).finished();
	Eigen::MatrixXi FA = (Eigen::MatrixXi(12, 3) <<
		0, 6, 4,
		0, 2, 6,
		0, 3, 2,
		0, 1, 3,
		2, 7, 6,
		2, 3, 7,
		4, 6, 7,
		4, 7, 5,
		0, 4, 5,
		0, 5, 1,
		1, 5, 7,
		1, 7, 3).finished();

	Eigen::MatrixXd VB = (Eigen::MatrixXd(8, 3) <<
		0.5, 0.0, 0.0,
		0.5, 0.0, 1.0,
		0.5, 1.0, 0.0,
		0.5, 1.0, 1.0,
		1.5, 0.0, 0.0,
		1.5, 0.0, 1.0,
		1.5, 1.0, 0.0,
		1.5, 1.0, 1.0).finished();
	Eigen::MatrixXi FB = (Eigen::MatrixXi(12, 3) <<
		0, 6, 4,
		0, 2, 6,
		0, 3, 2,
		0, 1, 3,
		2, 7, 6,
		2, 3, 7,
		4, 6, 7,
		4, 7, 5,
		0, 4, 5,
		0, 5, 1,
		1, 5, 7,
		1, 7, 3).finished();

	// 执行布尔运算
	Eigen::MatrixXd VU;
	Eigen::MatrixXi FU;
    igl::copyleft::cgal::mesh_boolean(VA, FA, VB, FB, igl::MeshBooleanType::MESH_BOOLEAN_TYPE_MINUS, VU, FU);

    return 0;
}

static int BooleanTest() {
	// 读取两个网格模型
	Eigen::MatrixXd VA, VB;
	Eigen::MatrixXi FA, FB;

	// time 1
	auto time1 = std::chrono::system_clock::now();

	igl::readOFF("C:/source/mesh_files/cheburashka.off", VA, FA);
	igl::readOFF("C:/source/mesh_files/decimated-knight.off", VB, FB);

	// time 2
	auto time2 = std::chrono::system_clock::now();
	std::cout << "Read Files: " << std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1).count() << " ms" << std::endl;

	// 执行布尔运算
	Eigen::MatrixXd VU;
	Eigen::MatrixXi FU;

	igl::copyleft::cgal::mesh_boolean(VA, FA, VB, FB, igl::MeshBooleanType::MESH_BOOLEAN_TYPE_UNION, VU, FU);
	std::cout << "VA.rows(): " << VA.rows() << std::endl;
	std::cout << "FA.rows(): " << FA.rows() << std::endl;
	std::cout << "VB.rows(): " << VB.rows() << std::endl;
	std::cout << "FB.rows(): " << FB.rows() << std::endl;

	// time 3
	auto time3 = std::chrono::system_clock::now();
	std::cout << "Do boolean: " << std::chrono::duration_cast<std::chrono::milliseconds>(time3 - time2).count() << " ms" << std::endl;

	// Plot the mesh with pseudocolors
	igl::opengl::glfw::Viewer viewer;
	viewer.data().set_mesh(VU, FU);

	viewer.data().show_lines = true;
	viewer.core().camera_dnear = 3.9;
	viewer.launch();

	return 0;
}

static int PlaneCutTest() {
	// 读取两个网格模型
	Eigen::MatrixXd VA, VB;
	Eigen::MatrixXi FA, FB;

	// time 1
	auto time1 = std::chrono::system_clock::now();

	// 待切割模型
	igl::readOFF("C:/source/mesh_files/cheburashka.off", VA, FA);

	// 创建平面
	VB = (Eigen::MatrixXd(4, 3) <<
		0.1, 0.5, 0.1,
		0.1, 0.5, 0.9,
		0.9, 0.5, 0.9,
		0.9, 0.5, 0.1).finished();
	FB = (Eigen::MatrixXi(2, 3) <<
		0, 1, 2,
		0, 2, 3).finished();


	// time 2
	auto time2 = std::chrono::system_clock::now();
	std::cout << "Read Files: " << std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1).count() << " ms" << std::endl;

	// 执行布尔运算
	Eigen::MatrixXd VU;
	Eigen::MatrixXi FU;

	igl::copyleft::cgal::mesh_boolean(VA, FA, VB, FB, igl::MeshBooleanType::MESH_BOOLEAN_TYPE_MINUS, VU, FU);
	std::cout << "VA.rows(): " << VA.rows() << std::endl;
	std::cout << "FA.rows(): " << FA.rows() << std::endl;
	std::cout << "VB.rows(): " << VB.rows() << std::endl;
	std::cout << "FB.rows(): " << FB.rows() << std::endl;

	// time 3
	auto time3 = std::chrono::system_clock::now();
	std::cout << "Do boolean: " << std::chrono::duration_cast<std::chrono::milliseconds>(time3 - time2).count() << " ms" << std::endl;

	// Plot the mesh with pseudocolors
	igl::opengl::glfw::Viewer viewer;
	viewer.data().set_mesh(VU, FU);
	viewer.launch();

	return 0;
}