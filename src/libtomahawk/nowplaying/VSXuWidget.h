#ifndef VSXuWidget_H
#define VSXuWidget_H

#include <vsx_manager.h>
#include <QGLWidget>
#include <QTimer>
#include <QStringList>

class VSXuWidget : public QGLWidget
{
  Q_OBJECT
private:
  vsx_manager_abs* m_manager;
  QTimer *m_timer; // A Timer to frequently update the display
  
  QStringList m_visuals;
  int m_width,m_height;
  bool m_isActive; // To Save the CPU when we aren't using VSXu.

protected:
  void initializeGL();
  void resizeGL(int w, int h){
    m_width = w;
    m_height = h;
  }
  void paintGL();
  
public:
  VSXuWidget(QWidget *parent = NULL);
  ~VSXuWidget();
  void injectSound(float soundData[]);
};

#endif // VSXuWidget_H
