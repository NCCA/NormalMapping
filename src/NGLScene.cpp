#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Texture.h>
#include <ngl/Obj.h>


NGLScene::NGLScene()
{
  setTitle("Normal mapping Demo");
  m_showNormals=false;
}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL( int _w, int _h )
{
  m_project=ngl::perspective(45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::initialize();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,0,2);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,1,0);
  // create our camera
  m_view=ngl::lookAt(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project=ngl::perspective(45.0f,(float)width()/height(),0.05f,350.0f);

  // now to load the shader and set the values
  // load a frag and vert shaders

  ngl::ShaderLib::createShaderProgram("TextureShader");

  ngl::ShaderLib::attachShader("SimpleVertex",ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("SimpleFragment",ngl::ShaderType::FRAGMENT);
  ngl::ShaderLib::loadShaderSource("SimpleVertex","shaders/NormalMapVert.glsl");
  ngl::ShaderLib::loadShaderSource("SimpleFragment","shaders/NormalMapFrag.glsl");

	ngl::ShaderLib::compileShader("SimpleVertex");
	ngl::ShaderLib::compileShader("SimpleFragment");
	ngl::ShaderLib::attachShaderToProgram("TextureShader","SimpleVertex");
	ngl::ShaderLib::attachShaderToProgram("TextureShader","SimpleFragment");

	// link the shader no attributes are bound
	ngl::ShaderLib::linkProgramObject("TextureShader");
	ngl::ShaderLib::use("TextureShader");
	// set our samplers for each of the textures this will correspond to the
	// multitexture id below
  ngl::ShaderLib::setUniform("diffuseMap",0); // was tex
	ngl::ShaderLib::setUniform("spec",1);
	ngl::ShaderLib::setUniform("normalMap",2);
  ngl::ShaderLib::setUniform("viewPos",from);
	// build our VertexArrayObject from the mesh
	loadModel();

  // load and set a texture for the colour
  ngl::Texture t("textures/TrollColour.png");
  t.setMultiTexture(0);
  t.setTextureGL();
  // mip map the textures
  glGenerateMipmap(GL_TEXTURE_2D);
  // now one for the specular map
  ngl::Texture spec("textures/2k_troll_spec_map.jpg");
  spec.setMultiTexture(1);
  spec.setTextureGL();
  // mip map the textures
  glGenerateMipmap(GL_TEXTURE_2D);
  // this is our normal map
  ngl::Texture normal("textures/2k_ct_normal.png");
  normal.setMultiTexture(2);
  normal.setTextureGL();
  // mip map the textures
  glGenerateMipmap(GL_TEXTURE_2D);

  glEnable(GL_MULTISAMPLE);

  /// now setup a basic 3 point lighting system
  m_key.position=ngl::Vec3(3,2,2);
  ngl::ShaderLib::setUniform("light[0].position",m_key.position);
  ngl::ShaderLib::setUniform("light[0].ambient",m_key.ambient);
  ngl::ShaderLib::setUniform("light[0].diffuse",m_key.diffuse);
  ngl::ShaderLib::setUniform("light[0].specular",m_key.specular);

  m_fill.position=ngl::Vec3(-3.0f,1.5f,2.0f);
  ngl::ShaderLib::setUniform("light[1].position",m_fill.position);
  ngl::ShaderLib::setUniform("light[1].ambient",m_fill.ambient);
  ngl::ShaderLib::setUniform("light[1].diffuse",m_fill.diffuse);
  ngl::ShaderLib::setUniform("light[1].specular",m_fill.specular);

  m_back.position=ngl::Vec3(0.0f,5.0f,0.0f);
  ngl::ShaderLib::setUniform("light[2].position",m_back.position);
  ngl::ShaderLib::setUniform("light[2].ambient",m_back.ambient);
  ngl::ShaderLib::setUniform("light[2].diffuse",m_back.diffuse);
  ngl::ShaderLib::setUniform("light[2].specular",m_back.specular);

  ngl::ShaderLib::createShaderProgram("normalShader");

  ngl::ShaderLib::attachShader("normalVertex",ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("normalFragment",ngl::ShaderType::FRAGMENT);
  ngl::ShaderLib::loadShaderSource("normalVertex","shaders/normalVertex.glsl");
  ngl::ShaderLib::loadShaderSource("normalFragment","shaders/normalFragment.glsl");

  ngl::ShaderLib::compileShader("normalVertex");
  ngl::ShaderLib::compileShader("normalFragment");
  ngl::ShaderLib::attachShaderToProgram("normalShader","normalVertex");
  ngl::ShaderLib::attachShaderToProgram("normalShader","normalFragment");

  ngl::ShaderLib::attachShader("normalGeo",ngl::ShaderType::GEOMETRY);
  ngl::ShaderLib::loadShaderSource("normalGeo","shaders/normalGeo.glsl");
  ngl::ShaderLib::compileShader("normalGeo");
  ngl::ShaderLib::attachShaderToProgram("normalShader","normalGeo");


  ngl::ShaderLib::linkProgramObject("normalShader");
  ngl::ShaderLib::use("normalShader");
  // now pass the modelView and projection values to the shader
  ngl::ShaderLib::setUniform("normalSize",0.01f);
}


// a simple structure to hold our vertex data
struct vertData
{
  GLfloat x; // position from obj
  GLfloat y;
  GLfloat z;
  GLfloat nx; // normal from obj mesh
  GLfloat ny;
  GLfloat nz;
  GLfloat u; // tex cords from obj
	GLfloat v; // tex cords
	GLfloat tx; // tangent calculated by us
	GLfloat ty;
	GLfloat tz;
	GLfloat bx; // binormal (bi-tangent really) calculated by us
	GLfloat by;
	GLfloat bz;
};


void NGLScene::loadModel()
{
	std::cout<<"loading mesh\n";
	// load the obj
	ngl::Obj mesh("models/troll.obj");
	std::cout<<"checking triangular\n";
	// this is only going to work for tri meshes so check
	if( ! mesh.isTriangular() )
	{
		std::cout<<"only works for tri meshes\n";
		exit(EXIT_FAILURE);
	}
	std::cout<<"getting lists\n";

  // get the obj data so we can process it locally
  std::vector <ngl::Vec3> verts=mesh.getVertexList();
  std::vector <ngl::Face> faces=mesh.getFaceList();
  std::vector <ngl::Vec3> tex=mesh.getUVList();
  std::vector <ngl::Vec3> normals=mesh.getNormalList();

	std::cout<<"got mesh data\n";
	// now we are going to process and pack the mesh into an ngl::VertexArrayObject
	std::vector <vertData> vboMesh;
	vertData d;
	auto nNorm=normals.size();
	auto nTex=tex.size();
	// loop for each of the faces
	for(auto f : faces)
	{
		// now for each triangle in the face (remember we ensured tri above)
    for(size_t j=0;j<3;++j)
		{
			// pack in the vertex data first
			d.x=verts[f.m_vert[j]].m_x;
			d.y=verts[f.m_vert[j]].m_y;
			d.z=verts[f.m_vert[j]].m_z;
			// now if we have norms of tex (possibly could not) pack them as well
			if(nNorm >0 && nTex > 0)
			{
				d.nx=normals[f.m_norm[j]].m_x;
				d.ny=normals[f.m_norm[j]].m_y;
				d.nz=normals[f.m_norm[j]].m_z;

        d.u=tex[f.m_uv[j]].m_x;
        d.v=tex[f.m_uv[j]].m_y;

      }
      // now if neither are present (only verts like Zbrush models)
      else if(nNorm ==0 && nTex==0)
      {
        d.nx=0;
        d.ny=0;
        d.nz=0;
        d.u=0;
        d.v=0;
      }
      // here we've got norms but not tex
      else if(nNorm >0 && nTex==0)
      {
        d.nx=normals[f.m_norm[j]].m_x;
        d.ny=normals[f.m_norm[j]].m_y;
        d.nz=normals[f.m_norm[j]].m_z;
        d.u=0;
        d.v=0;

      }
      // here we've got tex but not norm least common
      else if(nNorm ==0 && nTex>0)
      {
        d.nx=0;
        d.ny=0;
        d.nz=0;
        d.u=tex[f.m_uv[j]].m_x;
        d.v=tex[f.m_uv[j]].m_y;
      }
    // now we calculate the tangent / bi-normal (tangent) based on the article here
    // http://www.terathon.com/code/tangent.html

    ngl::Vec3 c1 = normals[f.m_norm[j]].cross(ngl::Vec3(0.0, 0.0, 1.0));
    ngl::Vec3 c2 = normals[f.m_norm[j]].cross(ngl::Vec3(0.0, 1.0, 0.0));
    ngl::Vec3 tangent;
    ngl::Vec3 binormal;
    if(c1.length()>c2.length())
    {
      tangent = c1;
    }
    else
    {
      tangent = c2;
    }
    // now we normalize the tangent so we don't need to do it in the shader
    tangent.normalize();
    // now we calculate the binormal using the model normal and tangent (cross)
    binormal = normals[f.m_norm[j]].cross(tangent);
    // normalize again so we don't need to in the shader
    binormal.normalize();
    d.tx=tangent.m_x;
    d.ty=tangent.m_y;
    d.tz=tangent.m_z;
    d.bx=binormal.m_x;
    d.by=binormal.m_y;
    d.bz=binormal.m_z;
    // finally add it to our mesh VAO structure
    vboMesh.push_back(d);
    }
  }

	// first we grab an instance of our VOA class as a TRIANGLE_STRIP
  m_vaoMesh= ngl::VAOFactory::createVAO(ngl::simpleVAO,GL_TRIANGLES);
	// next we bind it so it's active for setting data
	m_vaoMesh->bind();
  auto meshSize=vboMesh.size();
	// now we have our data add it to the VAO, we need to tell the VAO the following
	// how much (in bytes) data we are copying
	// a pointer to the first element of data (in this case the address of the first element of the
	// std::vector
  m_vaoMesh->setData(ngl::AbstractVAO::VertexData(meshSize*sizeof(vertData),vboMesh[0].x));
  m_vaoMesh->setVertexAttributePointer(0,3,GL_FLOAT,sizeof(vertData),0);
  m_vaoMesh->setVertexAttributePointer(1,3,GL_FLOAT,sizeof(vertData),3);
  m_vaoMesh->setVertexAttributePointer(2,2,GL_FLOAT,sizeof(vertData),6);
	// tangent same as vertex only starts at position 8 (u,v)-> nx
	m_vaoMesh->setVertexAttributePointer(3,3,GL_FLOAT,sizeof(vertData),8);
	// bi-tangent (or Binormal) same as vertex only starts at position 11 (u,v)-> nx
	m_vaoMesh->setVertexAttributePointer(4,3,GL_FLOAT,sizeof(vertData),11);
	// now we have set the vertex attributes we tell the VAO class how many indices to draw when
	// glDrawArrays is called, in this case we use buffSize (but if we wished less of the sphere to be drawn we could
	// specify less (in steps of 3))
	m_vaoMesh->setNumIndices(meshSize);
	// finally we have finished for now so time to unbind the VAO
	m_vaoMesh->unbind();

}



void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib::use("TextureShader");
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat4 M;
  M=m_mouseGlobalTX*m_transform.getMatrix();
  MV=m_view*M;
  MVP=m_project*MV ;
  ngl::Mat3 normalMatrix=M;
  normalMatrix.inverse().transpose();
  ngl::ShaderLib::setUniform("MVP",MVP);
  ngl::ShaderLib::setUniform("normalMatrix",normalMatrix);
  ngl::ShaderLib::setUniform("M",M);

}


void NGLScene::loadMatricesToNormalShader()
{
  ngl::ShaderLib::use("normalShader");
  ngl::Mat4 MV;
  ngl::Mat4 MVP;

  MV= m_view*
      m_mouseGlobalTX*
      m_transform.getMatrix()
      ;
  MVP=m_project*MV;
  ngl::ShaderLib::setUniform("MVP",MVP);

}


void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_win.width,m_win.height);
  // Rotation based on the mouse position for our global
  // transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_win.spinXFace);
  rotY.rotateY(m_win.spinYFace);
  // multiply the rotations
  m_mouseGlobalTX=rotY*rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;
  // set this in the TX stack
  loadMatricesToShader();
  // now we bind back our vertex array object and draw
  m_vaoMesh->bind();
  // draw the VAO
  m_vaoMesh->draw();
  if(m_showNormals == true)
  {
    loadMatricesToNormalShader();
    m_vaoMesh->draw();
  }
  // now we are done so unbind
  m_vaoMesh->unbind();
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  enum class Mode : bool {Enable,Disable};
  auto setLight=[](const std::string &_name, Mode _mode)
  {
    if(_mode==Mode::Enable)
    {
      ngl::ShaderLib::setUniform(fmt::format("{0}.ambient",_name),ngl::Vec3(0.01f,0.01f,0.01f));
      ngl::ShaderLib::setUniform(fmt::format("{0}.diffuse",_name),ngl::Vec3(1.0f,1.0f,1.0f));
      ngl::ShaderLib::setUniform(fmt::format("{0}.specular",_name),ngl::Vec3(1.0f,1.0f,1.0f));
    }
    else
    {
      ngl::ShaderLib::setUniform(fmt::format("{0}.ambient",_name),ngl::Vec3::zero());
      ngl::ShaderLib::setUniform(fmt::format("{0}.diffuse",_name),ngl::Vec3::zero());
      ngl::ShaderLib::setUniform(fmt::format("{0}.specular",_name),ngl::Vec3::zero());
    }
  };
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wireframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  case Qt::Key_1 : setLight("light[0]",Mode::Enable); break;
  case Qt::Key_2 : setLight("light[0]",Mode::Disable); break;
  case Qt::Key_3 : setLight("light[1]",Mode::Enable); break;
  case Qt::Key_4 : setLight("light[1]",Mode::Disable); break;
  case Qt::Key_5 : setLight("light[2]",Mode::Enable); break;
  case Qt::Key_6 : setLight("light[2]",Mode::Disable); break;
  case Qt::Key_T : m_showNormals^=true; break;
  case Qt::Key_Space :
        m_win.spinXFace=0;
        m_win.spinYFace=0;
        m_modelPos.set(ngl::Vec3::zero());
  break;
  default : break;
  }
  // finally update the GLWindow and re-draw
 update();
}
