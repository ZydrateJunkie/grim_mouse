// Residual - Virtual machine to run LucasArts' 3D adventure games
// Copyright (C) 2003-2004 The ScummVM-Residual Team (www.scummvm.org)
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

#include "stdafx.h"
#include "model.h"
#include "bits.h"
#include "resource.h"
#include "material.h"
#include "debug.h"
#include "textsplit.h"
#include <cstring>
#include <SDL.h>
#include <SDL_opengl.h>
#include "screen.h"

Model::Model(const char *filename, const char *data, int len,
			 const CMap &cmap) : Resource(filename) {
	if (len >= 4 && std::memcmp(data, "LDOM", 4) == 0)
		loadBinary(data, cmap);
	else {
		TextSplitter ts(data, len);
		loadText(ts, cmap);
	}
}

void Model::loadBinary(const char *data, const CMap &cmap) {
	numMaterials_ = READ_LE_UINT32(data + 4);
	data += 8;
	materials_ = new ResPtr<Material>[numMaterials_];
	for (int i = 0; i < numMaterials_; i++) {
		materials_[i] = ResourceLoader::instance()->loadMaterial(data, cmap);
		data += 32;
	}
	data += 32; // skip name
	numGeosets_ = READ_LE_UINT32(data + 4);
	data += 8;
	geosets_ = new Geoset[numGeosets_];
	for (int i = 0; i < numGeosets_; i++)
		geosets_[i].loadBinary(data, materials_);
	numHierNodes_ = READ_LE_UINT32(data + 4);
	data += 8;
	rootHierNode_ = new HierNode[numHierNodes_];
	for (int i = 0; i < numHierNodes_; i++)
		rootHierNode_[i].loadBinary(data, rootHierNode_, geosets_[0]);
	radius_ = get_float(data);
	insertOffset_ = get_vector3d(data + 40);
}

Model::~Model() {
	delete[] materials_;
	delete[] geosets_;
	delete[] rootHierNode_;
}

void Model::Geoset::loadBinary(const char *&data, ResPtr<Material> *materials) {
	numMeshes_ = READ_LE_UINT32(data);
	data += 4;
	meshes_ = new Mesh[numMeshes_];
	for (int i = 0; i < numMeshes_; i++)
		meshes_[i].loadBinary(data, materials);
}

Model::Geoset::~Geoset() {
	delete[] meshes_;
}

void Model::Mesh::loadBinary(const char *&data, ResPtr<Material> *materials) {
	memcpy(name_, data, 32);
	geometryMode_ = READ_LE_UINT32(data + 36);
	lightingMode_ = READ_LE_UINT32(data + 40);
	textureMode_ = READ_LE_UINT32(data + 44);
	numVertices_ = READ_LE_UINT32(data + 48);
	numTextureVerts_ = READ_LE_UINT32(data + 52);
	numFaces_ = READ_LE_UINT32(data + 56);
	vertices_ = new float[3 * numVertices_];
	verticesI_ = new float[numVertices_];
	vertNormals_ = new float[3 * numVertices_];
	textureVerts_ = new float[2 * numTextureVerts_];
	data += 60;
	for (int i = 0; i < 3 * numVertices_; i++) {
		vertices_[i] = get_float(data);
		data += 4;
	}
	for (int i = 0; i < 2 * numTextureVerts_; i++) {
		textureVerts_[i] = get_float(data);
		data += 4;
	}
	for (int i = 0; i < numVertices_; i++) {
		verticesI_[i] = get_float(data);
		data += 4;
	}
	data += numVertices_ * 4;
	faces_ = new Face[numFaces_];
	for (int i = 0; i < numFaces_; i++)
		faces_[i].loadBinary(data, materials);
	vertNormals_ = new float[3 * numVertices_];
	for (int i = 0; i < 3 * numVertices_; i++) {
		vertNormals_[i] = get_float(data);
		data += 4;
	}
	shadow_ = READ_LE_UINT32(data);
	radius_ = get_float(data + 8);
	data += 36;
}

Model::Mesh::~Mesh() {
	delete[] vertices_;
	delete[] verticesI_;
	delete[] vertNormals_;
	delete[] textureVerts_;
	delete[] faces_;
}

void Model::Mesh::update() {
	GLdouble modelView[500];
	GLdouble projection[500];
	GLint viewPort[500];

	glGetDoublev( GL_MODELVIEW_MATRIX, modelView );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewPort);

	GLdouble top = 1000;
	GLdouble right = -1000;
	GLdouble left = 1000;
	GLdouble bottom = -1000;

	for (int i = 0; i < numFaces_; i++) {
		Vector3d v;
		Matrix4 tempMatrix = matrix_;
		float* pVertices;
		int j;
		float bestDepth = 0;

		for( j =0; j< faces_[i].numVertices_; j++ ) {
			pVertices = vertices_ + 3 * faces_[i].vertices_[j];

			v.set( *(pVertices), *(pVertices+1), *(pVertices+2) );

			tempMatrix.rot_.transform( &v );
			v+= tempMatrix.pos_;

			GLdouble winX;
			GLdouble winY;
			GLdouble winZ;

			gluProject( v.x(), v.y(), v.z(), modelView, projection, viewPort, &winX, &winY, &winZ);

			if( winX > right )
				right = winX;
			if( winX < left )
				left = winX;
			if( winY < top )
				top = winY;
			if( winY > bottom )
				bottom = winY;

			if( winZ> bestDepth )
				bestDepth = winZ;

		}

		//screenBlocksAddRectangle( top, right, left, bottom, bestDepth );
	}

	glDisable(GL_DEPTH_TEST);
	glPointSize( 3.f );
	glColor4f( 1.f, 1.f, 0.f, 1.f );
	glDisable(GL_TEXTURE_2D );
}

void Model::Face::loadBinary(const char *&data, ResPtr<Material> *materials) {
	type_ = READ_LE_UINT32(data + 4);
	geo_ = READ_LE_UINT32(data + 8);
	light_ = READ_LE_UINT32(data + 12);
	tex_ = READ_LE_UINT32(data + 16);
	numVertices_ = READ_LE_UINT32(data + 20);
	int texPtr = READ_LE_UINT32(data + 28);
	int materialPtr = READ_LE_UINT32(data + 32);
	extraLight_ = get_float(data + 48);
	normal_ = get_vector3d(data + 64);
	data += 76;

	vertices_ = new int[numVertices_];
	for (int i = 0; i < numVertices_; i++) {
		vertices_[i] = READ_LE_UINT32(data);
		data += 4;
	}
	if (texPtr == 0)
		texVertices_ = NULL;
	else {
		texVertices_ = new int[numVertices_];
		for (int i = 0; i < numVertices_; i++) {
			texVertices_[i] = READ_LE_UINT32(data);
			data += 4;
		}
	}
	if (materialPtr == 0)
		material_ = 0;
	else {
		material_ = materials[READ_LE_UINT32(data)];
		data += 4;
	}
}

Model::Face::~Face() {
	delete[] vertices_;
	delete[] texVertices_;
}

void Model::HierNode::loadBinary(const char *&data,
	Model::HierNode *hierNodes,
	const Geoset &g) {
		memcpy(name_, data, 64);
		flags_ = READ_LE_UINT32(data + 64);
		type_ = READ_LE_UINT32(data + 72);
		int meshNum = READ_LE_UINT32(data + 76);
		if (meshNum < 0)
			mesh_ = NULL;
		else
			mesh_ = g.meshes_ + meshNum;
		depth_ = READ_LE_UINT32(data + 80);
		int parentPtr = READ_LE_UINT32(data + 84);
		numChildren_ = READ_LE_UINT32(data + 88);
		int childPtr = READ_LE_UINT32(data + 92);
		int siblingPtr = READ_LE_UINT32(data + 96);
		pivot_ = get_vector3d(data + 100);
		pos_ = get_vector3d(data + 112);
		pitch_ = get_float(data + 124);
		yaw_ = get_float(data + 128);
		roll_ = get_float(data + 132);
		animPos_ = pos_;
		animPitch_ = pitch_;
		animYaw_ = yaw_;
		animRoll_ = roll_;
		priority_ = -1;
		totalWeight_ = 1;

		data += 184;

		if (parentPtr != 0) {
			parent_ = hierNodes + READ_LE_UINT32(data);
			data += 4;
		} else
			parent_ = NULL;
		if (childPtr != 0) {
			child_ = hierNodes + READ_LE_UINT32(data);
			data += 4;
		} else
			child_ = NULL;
		if (siblingPtr != 0) {
			sibling_ = hierNodes + READ_LE_UINT32(data);
			data += 4;
		} else
			sibling_ = NULL;

		meshVisible_ = hierVisible_ = true;
		totalWeight_ = 1;
	}

void Model::draw() const {
	rootHierNode_->draw();
}

Model::HierNode *Model::copyHierarchy() {
	HierNode *result = new HierNode[numHierNodes_];
	std::memcpy(result, rootHierNode_, numHierNodes_ * sizeof(HierNode));
	// Now adjust pointers
	for (int i = 0; i < numHierNodes_; i++) {
		if (result[i].parent_ != NULL)
			result[i].parent_ = result + (rootHierNode_[i].parent_ - rootHierNode_);
		if (result[i].child_ != NULL)
			result[i].child_ = result + (rootHierNode_[i].child_ - rootHierNode_);
		if (result[i].sibling_ != NULL)
			result[i].sibling_ = result + (rootHierNode_[i].sibling_ -
		rootHierNode_);
	}
	return result;
}

void Model::loadText(TextSplitter &ts, const CMap &cmap) {
	ts.expectString("section: header");
	int major, minor;
	ts.scanString("3do %d.%d", 2, &major, &minor);

	ts.expectString("section: modelresource");
	ts.scanString("materials %d", 1, &numMaterials_);
	materials_ = new ResPtr<Material>[numMaterials_];
	for (int i = 0; i < numMaterials_; i++) {
		int num;
		char name[32];
		ts.scanString("%d: %32s", 2, &num, name);
		materials_[num] = ResourceLoader::instance()->loadMaterial(name, cmap);
	}

	ts.expectString("section: geometrydef");
	ts.scanString("radius %f", 1, &radius_);
	ts.scanString("insert offset %f %f %f", 3,
		&insertOffset_.x(), &insertOffset_.y(), &insertOffset_.z());
	ts.scanString("geosets %d", 1, &numGeosets_);
	geosets_ = new Geoset[numGeosets_];
	for (int i = 0; i < numGeosets_; i++) {
		int num;
		ts.scanString("geoset %d", 1, &num);
		geosets_[num].loadText(ts, materials_);
	}

	ts.expectString("section: hierarchydef");
	ts.scanString("hierarchy nodes %d", 1, &numHierNodes_);
	rootHierNode_ = new HierNode[numHierNodes_];
	for (int i = 0; i < numHierNodes_; i++) {
		int num, flags, type, mesh, parent, child, sibling, numChildren;
		float x, y, z, pitch, yaw, roll, pivotx, pivoty, pivotz;
		char name[64];
		ts.scanString(" %d: %i %i %d %d %d %d %d %f %f %f %f %f %f %f %f %f %64s",
			18, &num, &flags, &type, &mesh, &parent, &child, &sibling,
			&numChildren, &x, &y, &z, &pitch, &yaw, &roll,
			&pivotx, &pivoty, &pivotz, name);
		rootHierNode_[num].flags_ = flags;
		rootHierNode_[num].type_ = type;
		if (mesh < 0)
			rootHierNode_[num].mesh_ = NULL;
		else
			rootHierNode_[num].mesh_ = geosets_[0].meshes_ + mesh;
		if (parent >= 0) {
			rootHierNode_[num].parent_ = rootHierNode_ + parent;
			rootHierNode_[num].depth_ = rootHierNode_[parent].depth_ + 1;
		} else {
			rootHierNode_[num].parent_ = NULL;
			rootHierNode_[num].depth_ = 0;
		}
		if (child >= 0)
			rootHierNode_[num].child_ = rootHierNode_ + child;
		else
			rootHierNode_[num].child_ = NULL;
		if (sibling >= 0)
			rootHierNode_[num].sibling_ = rootHierNode_ + sibling;
		else
			rootHierNode_[num].sibling_ = NULL;
		rootHierNode_[num].numChildren_ = numChildren;
		rootHierNode_[num].pos_ = Vector3d(x, y, z);
		rootHierNode_[num].pitch_ = pitch;
		rootHierNode_[num].yaw_ = yaw;
		rootHierNode_[num].roll_ = roll;
		rootHierNode_[num].pivot_ = Vector3d(pivotx, pivoty, pivotz);

		rootHierNode_[num].meshVisible_ =
		rootHierNode_[num].hierVisible_ = true;
		rootHierNode_[num].totalWeight_ = 1;
	}
	if (! ts.eof())
	warning("Unexpected junk at end of model text\n");
}

void Model::Geoset::loadText(TextSplitter &ts, ResPtr<Material> *materials) {
	ts.scanString("meshes %d", 1, &numMeshes_);
	meshes_ = new Mesh[numMeshes_];
	for (int i = 0; i < numMeshes_; i++) {
		int num;
		ts.scanString("mesh %d", 1, &num);
		meshes_[num].loadText(ts, materials);
	}
}

void Model::Mesh::loadText(TextSplitter &ts, ResPtr<Material> *materials) {
	ts.scanString("name %32s", 1, name_);
	ts.scanString("radius %f", 1, &radius_);

	// In data001/rope_scale.3do, the shadow line is missing
	if (std::sscanf(ts.currentLine(), "shadow %d", &shadow_) < 1) {
		shadow_ = 0;
		warning("Missing shadow directive in model\n");
	} else
		ts.nextLine();
	ts.scanString("geometrymode %d", 1, &geometryMode_);
	ts.scanString("lightingmode %d", 1, &lightingMode_);
	ts.scanString("texturemode %d", 1, &textureMode_);
	ts.scanString("vertices %d", 1, &numVertices_);
	vertices_ = new float[3 * numVertices_];
	verticesI_ = new float[numVertices_];
	vertNormals_ = new float[3 * numVertices_];

	for (int i = 0; i < numVertices_; i++) {
		int num;
		float x, y, z, ival;
		ts.scanString(" %d: %f %f %f %f", 5, &num, &x, &y, &z, &ival);
		vertices_[3 * num] = x;
		vertices_[3 * num + 1] = y;
		vertices_[3 * num + 2] = z;
		verticesI_[num] = ival;
	}

	ts.scanString("texture vertices %d", 1, &numTextureVerts_);
	textureVerts_ = new float[2 * numTextureVerts_];

	for (int i = 0; i < numTextureVerts_; i++) {
		int num;
		float x, y;
		ts.scanString(" %d: %f %f", 3, &num, &x, &y);
		textureVerts_[2 * num] = x;
		textureVerts_[2 * num + 1] = y;
	}

	ts.expectString("vertex normals");
	for (int i = 0; i < numVertices_; i++) {
		int num;
		float x, y, z;
		ts.scanString(" %d: %f %f %f", 4, &num, &x, &y, &z);
		vertNormals_[3 * num] = x;
		vertNormals_[3 * num + 1] = y;
		vertNormals_[3 * num + 2] = z;
	}

	ts.scanString("faces %d", 1, &numFaces_);
	faces_ = new Face[numFaces_];
	for (int i = 0; i < numFaces_; i++) {
		int num, material, type, geo, light, tex, verts;
		float extralight;
		int readlen;
		if (ts.eof())
			error("Expected face data, got EOF\n");
		if (std::sscanf(ts.currentLine(), " %d: %d %i %d %d %d %f %d%n",
			&num, &material, &type, &geo, &light, &tex, &extralight,
			&verts, &readlen) < 8)
				error("Expected face data, got `%s'\n", ts.currentLine());
		faces_[num].material_ = materials[material];
		faces_[num].type_ = type;
		faces_[num].geo_ = geo;
		faces_[num].light_ = light;
		faces_[num].tex_ = tex;
		faces_[num].extraLight_ = extralight;
		faces_[num].numVertices_ = verts;
		faces_[num].vertices_ = new int[verts];
		faces_[num].texVertices_ = new int[verts];
		for (int j = 0; j < verts; j++) {
			int readlen2;
			if (std::sscanf(ts.currentLine() + readlen, " %d, %d%n",
				faces_[num].vertices_ + j,
				faces_[num].texVertices_ + j, &readlen2) < 2)
					error("Could not read vertex indices in line `%s'\n",
			ts.currentLine());
			readlen += readlen2;
		}
		ts.nextLine();
	}

	ts.expectString("face normals");
	for (int i = 0; i < numFaces_; i++) {
		int num;
		float x, y, z;
		ts.scanString(" %d: %f %f %f", 4, &num, &x, &y, &z);
		faces_[num].normal_ = Vector3d(x, y, z);
	}
}

void Model::HierNode::draw() const {
	if (hierVisible_) {
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		glTranslatef(animPos_.x() / totalWeight_, animPos_.y() / totalWeight_,
			animPos_.z() / totalWeight_);
		glRotatef(animYaw_ / totalWeight_, 0, 0, 1);
		glRotatef(animPitch_ / totalWeight_, 1, 0, 0);
		glRotatef(animRoll_ / totalWeight_, 0, 1, 0);

		if (mesh_ != NULL && meshVisible_) {
			glPushMatrix();
			glTranslatef(pivot_.x(), pivot_.y(), pivot_.z());
			mesh_->draw();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
		if (child_ != NULL) {
			child_->draw();
			glMatrixMode(GL_MODELVIEW);
		}
		glPopMatrix();
	}
	if (sibling_ != NULL)
		sibling_->draw();
}

void Model::HierNode::addChild(HierNode *child) {
	HierNode **childPos = &child_;
	while (*childPos != NULL)
		childPos = &(*childPos)->sibling_;
	*childPos = child;
	child->parent_ = this;
}

void Model::HierNode::removeChild(HierNode *child) {
	HierNode **childPos = &child_;
	while (*childPos != NULL && *childPos != child)
		childPos = &(*childPos)->sibling_;
	if (*childPos != NULL) {
		*childPos = child->sibling_;
		child->parent_ = NULL;
	}
}

void Model::HierNode::setMatrix(Matrix4 matrix) {
	matrix_ = matrix;
}

void Model::HierNode::update() {

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glTranslatef(animPos_.x() / totalWeight_, animPos_.y() / totalWeight_,
		animPos_.z() / totalWeight_);
	glRotatef(animYaw_ / totalWeight_, 0, 0, 1);
	glRotatef(animPitch_ / totalWeight_, 1, 0, 0);
	glRotatef(animRoll_ / totalWeight_, 0, 1, 0);


	localMatrix_.pos_.set( animPos_.x() / totalWeight_, animPos_.y() / totalWeight_, animPos_.z() / totalWeight_ );
	localMatrix_.rot_.buildFromPitchYawRoll( animPitch_ / totalWeight_, animYaw_ / totalWeight_, animRoll_ / totalWeight_);

	matrix_ *= localMatrix_;

	pivotMatrix = matrix_;

	pivotMatrix.translate( pivot_.x(), pivot_.y(), pivot_.z() );

	if( mesh_ != NULL ) {
		glPushMatrix();
		glTranslatef(pivot_.x(), pivot_.y(), pivot_.z());
		mesh_->matrix_ = pivotMatrix;
		mesh_->update();

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	if( child_ != NULL ) {
		child_->setMatrix( matrix_ );
		child_->update();
		glMatrixMode(GL_MODELVIEW);
	}

	glPopMatrix();
}


void Model::Mesh::draw() const {
	for (int i = 0; i < numFaces_; i++)
		faces_[i].draw(vertices_, vertNormals_, textureVerts_);

	GLdouble modelView[500];
	GLdouble projection[500];
	GLint viewPort[500];

	glGetDoublev( GL_MODELVIEW_MATRIX, modelView );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewPort);

	// Yaz: debug
	// this draw the model node in red
	//glMatrixMode(GL_PROJECTION);
	/*glPushMatrix();
	glLoadIdentity();

	GLdouble modelView[500];
	GLdouble projection[500];
	GLint viewPort[500];

	glGetDoublev( GL_MODELVIEW_MATRIX, modelView );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewPort);

	glDisable(GL_DEPTH_TEST);
	glPointSize( 3.f );
	glColor4f( 1.f, 0.f, 0.f, 1.f );
	glDisable(GL_TEXTURE_2D );
	glBegin( GL_POINTS );
	glVertex3f( matrix_.pos_.x(), matrix_.pos_.y(), matrix_.pos_.z() );
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D );*/

	// Yaz: debug
	// this draw the poly points

	/*glPushMatrix();
	glLoadIdentity();
	glPointSize( 3.f );
	glColor4f( 0.f, 1.f, 0.f, 1.f );
	glDisable(GL_TEXTURE_2D );
	{
		GLdouble modelView[500];
		GLdouble projection[500];
		GLint viewPort[500];

		glGetDoublev( GL_MODELVIEW_MATRIX, modelView );
		glGetDoublev( GL_PROJECTION_MATRIX, projection );
		glGetIntegerv( GL_VIEWPORT, viewPort);
	}

	glBegin( GL_POINTS );

	for (int i = 0; i < numFaces_; i++) {
		Vector3d v;
		Matrix4 tempMatrix = matrix_;
		float* pVertices;
		int j;

		for( j =0; j< faces_[i].numVertices_; j++ ) {
			pVertices = vertices_ + 3 * faces_[i].vertices_[j];

			v.set( *(pVertices), *(pVertices+1), *(pVertices+2) );

			tempMatrix.rot_.transform( &v );
			v+= tempMatrix.pos_;

			glVertex3f( v.x(), v.y(), v.z() );

		}
	}

	glEnd();
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D ); */

	// Ender: HACK HACK HACK
	// Mannys head isn't computed correctly, so bail out to prevent memory corruption.
	// at least until it IS computed, or the DirtyScreen code has bounds checking :)
	//if (strstr(name_, "m_head_1"))
	//	return;

	// Yaz: debug
	// this compute the dirty rect for the mesh
	glPushMatrix();
	glLoadIdentity();

	GLdouble top = 1000;
	GLdouble right = -1000;
	GLdouble left = 1000;
	GLdouble bottom = -1000;

	for (int i = 0; i < numFaces_; i++) {
		Vector3d v;
		Matrix4 tempMatrix = matrix_;
		float* pVertices;
		int j;
		float bestDepth = 0;

		for( j =0; j< faces_[i].numVertices_; j++ ) {
			GLdouble modelView[500];
			GLdouble projection[500];
			GLint viewPort[500];

			glGetDoublev( GL_MODELVIEW_MATRIX, modelView );
			glGetDoublev( GL_PROJECTION_MATRIX, projection );
			glGetIntegerv( GL_VIEWPORT, viewPort);

			pVertices = vertices_ + 3 * faces_[i].vertices_[j];

			v.set( *(pVertices), *(pVertices+1), *(pVertices+2) );

			tempMatrix.rot_.transform( &v );
			v+= tempMatrix.pos_;

			GLdouble winX;
			GLdouble winY;
			GLdouble winZ;

			gluProject( v.x(), v.y(), v.z(), modelView, projection, viewPort, &winX, &winY, &winZ);

			if( winX > right )
				right = winX;
			if( winX < left )
				left = winX;
			if( winY < top )
				top = winY;
			if( winY > bottom )
				bottom = winY;

			if( winZ> bestDepth )
				bestDepth = winZ;
		}

		if (SCREENBLOCKS_GLOBAL)
			screenBlocksAddRectangle( (int)top, (int)right, (int)left, (int)bottom, (int)bestDepth );
	}
	/*
	glDisable(GL_DEPTH_TEST);
	glPointSize( 3.f );
	glColor4f( 1.f, 1.f, 0.f, 1.f );
	glDisable(GL_TEXTURE_2D );

	glBegin(GL_LINES);

	GLdouble objx;
	GLdouble objy;
	GLdouble objz;

	// top
	gluUnProject( left, top, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );
	gluUnProject( right, top, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );

	// bottom
	gluUnProject( left, bottom, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );
	gluUnProject( right, bottom, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );

	// left
	gluUnProject( left, top, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );
	gluUnProject( left, bottom, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );

	// right
	gluUnProject( right, top, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );
	gluUnProject( right, bottom, 1.f, modelView, projection, viewPort, &objx, &objy, &objz );
	glVertex3f( objx, objy, objz );

	glEnd(); 
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D ); 
	*/
 	glPopMatrix();
	}

void Model::Face::draw(float *vertices, float *vertNormals, float *textureVerts) const {
	material_->select();
	glNormal3fv(normal_.coords_);
	glBegin(GL_POLYGON);
	for (int i = 0; i < numVertices_; i++) {
		glNormal3fv(vertNormals + 3 * vertices_[i]);
		if (texVertices_ != NULL)
			glTexCoord2fv(textureVerts + 2 * texVertices_[i]);
		glVertex3fv(vertices + 3 * vertices_[i]);
	}
	glEnd();
}
