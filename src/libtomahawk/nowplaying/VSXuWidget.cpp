#include <QDebug>

#include "VSXuWidget.h"

VSXuWidget::VSXuWidget(QWidget *parent):
  QGLWidget(parent),
  m_isActive(true)
{
  setWindowTitle("Vovoid VSXu");
  m_timer = new QTimer(this);
  connect (m_timer , SIGNAL(timeout()), this, SLOT(update()));
}

void VSXuWidget::injectSound(float soundData[])
{
  /* uncomment for manual sound injection
  for (unsigned long i = 0; i < 512; i++)
  {
    sound_wave_test[i] = (float)(rand()%65535-32768)*(1.0f/32768.0f);
  }
  for (unsigned long i = 0; i < 512; i++)
  {
    sound_freq_test[i] = (float)(rand()%65535)*(1.0f/65535.0f);
  }
  manager->set_sound_freq(&sound_freq_test[0]);
  manager->set_sound_wave(&sound_wave_test[0]);
  */

  updateGL();
}

void VSXuWidget::initializeGL()
{
  m_manager = manager_factory();
  // init manager with the shared path and sound input type.
  // manual sound injection: manager->init( path.c_str() , "media_player");
  m_manager->init(0 , "pulseaudio");
  
  std::vector<std::string> files = m_manager->get_visual_filenames();
  
  qDebug()<<files.size();
  for(int i = 0 ; i < files.size(); i++){
    m_visuals.push_back(QString::fromStdString(files[i]));
  }
  qDebug()<<m_visuals;
  
  glEnable(GL_BLEND);
  glEnable(GL_POLYGON_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0, 0, 0, 0);
  
  glViewport( 0, 0, width(), height());
  
  if (context()->format().hasOpenGL())
    qDebug()<<"You have a Valid OpenGL Context";
  if (context()->format().sampleBuffers())
    qDebug()<<"You have Sample Buffers";
  m_timer->start(20);
}


void VSXuWidget::paintGL()
{
  if (m_isActive){
    glViewport(0, 0, m_width, m_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, m_width, 0, m_height); // set origin to bottom left corner
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_manager)
      m_manager->render();
  }
}


VSXuWidget::~VSXuWidget()
{}