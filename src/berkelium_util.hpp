//================================================================================================
// Berkelium Utility
//
//
//
//
//================================================================================================
#ifndef _BERKELIUM_UTIL_HPP_
#define _BERKELIUM_UTIL_HPP_


//#if defined(GL_BGRA_EXT) && !defined(GL_BGRA)
//#define GL_BGRA GL_BGRA_EXT
//#endif
//================================================================================================
#include <GL/gl.h>
#include <GL/glu.h>
//================================================================================================
#include "berkelium/Berkelium.hpp"
#include "berkelium/Window.hpp"
#include "berkelium/ScriptUtil.hpp"
#include "berkelium/WindowDelegate.hpp"
#include "berkelium/Context.hpp"
//================================================================================================
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>
//================================================================================================

#include <sigc++/sigc++.h>

//================================================================================================
#define DEBUG_PAINT true

using namespace Berkelium;

/** Handles an onPaint call by mapping the results into an OpenGL texture. The
 *  first parameters are the same as Berkelium::WindowDelegate::onPaint.  The
 *  additional parameters and return value are:
 *  \param dest_texture - the OpenGL texture handle for the texture to render
 *                        the results into.
 *  \param dest_texture_width - width of destination texture
 *  \param dest_texture_height - height of destination texture
 *  \param ignore_partial - if true, ignore any partial updates.  This is useful
 *         if you have loaded a new page, but updates for the old page have not
 *         completed yet.
 *  \param scroll_buffer - a temporary workspace used for scroll data.  Must be
 *         at least dest_texture_width * dest_texture_height * 4 bytes large.
 *  \returns true if the texture was updated, false otherwiase
 */
bool mapOnPaintToTexture(
    Berkelium::Window *wini,
    const unsigned char* bitmap_in, const Berkelium::Rect& bitmap_rect,
    size_t num_copy_rects, const Berkelium::Rect *copy_rects,
    int dx, int dy,
    const Berkelium::Rect& scroll_rect,
    unsigned int dest_texture,
    unsigned int dest_texture_width,
    unsigned int dest_texture_height,
    bool ignore_partial,
    char* scroll_buffer) {

    glBindTexture(GL_TEXTURE_2D, dest_texture);

    const int kBytesPerPixel = 4;

    // If we've reloaded the page and need a full update, ignore updates
    // until a full one comes in.  This handles out of date updates due to
    // delays in event processing.
    if (ignore_partial) {
        if (bitmap_rect.left() != 0 ||
            bitmap_rect.top() != 0 ||
            bitmap_rect.right() != dest_texture_width ||
            bitmap_rect.bottom() != dest_texture_height) {
            return false;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, kBytesPerPixel, dest_texture_width, dest_texture_height, 0,
            GL_BGRA, GL_UNSIGNED_BYTE, bitmap_in);
        ignore_partial = false;
        return true;
    }


    // Now, we first handle scrolling. We need to do this first since it
    // requires shifting existing data, some of which will be overwritten by
    // the regular dirty rect update.
    if (dx != 0 || dy != 0) {
        // scroll_rect contains the Rect we need to move
        // First we figure out where the the data is moved to by translating it
        Berkelium::Rect scrolled_rect = scroll_rect.translate(-dx, -dy);
        // Next we figure out where they intersect, giving the scrolled
        // region
        Berkelium::Rect scrolled_shared_rect = scroll_rect.intersect(scrolled_rect);
        // Only do scrolling if they have non-zero intersection
        if (scrolled_shared_rect.width() > 0 && scrolled_shared_rect.height() > 0) {
            // And the scroll is performed by moving shared_rect by (dx,dy)
            Berkelium::Rect shared_rect = scrolled_shared_rect.translate(dx, dy);

            int wid = scrolled_shared_rect.width();
            int hig = scrolled_shared_rect.height();
            if (DEBUG_PAINT) {
              std::cout << "Scroll rect: w=" << wid << ", h=" << hig << ", ("
                        << scrolled_shared_rect.left() << "," << scrolled_shared_rect.top()
                        << ") by (" << dx << "," << dy << ")" << std::endl;
            }
            int inc = 1;
            char *outputBuffer = scroll_buffer;
            // source data is offset by 1 line to prevent memcpy aliasing
            // In this case, it can happen if dy==0 and dx!=0.
            char *inputBuffer = scroll_buffer+(dest_texture_width*1*kBytesPerPixel);
            int jj = 0;
            if (dy > 0) {
                // Here, we need to shift the buffer around so that we start in the
                // extra row at the end, and then copy in reverse so that we
                // don't clobber source data before copying it.
                outputBuffer = scroll_buffer+(
                    (scrolled_shared_rect.top()+hig+1)*dest_texture_width
                    - hig*wid)*kBytesPerPixel;
                inputBuffer = scroll_buffer;
                inc = -1;
                jj = hig-1;
            }

            // Copy the data out of the texture
            glGetTexImage(
                GL_TEXTURE_2D, 0,
                GL_BGRA, GL_UNSIGNED_BYTE,
                inputBuffer
            );

            // Annoyingly, OpenGL doesn't provide convenient primitives, so
            // we manually copy out the region to the beginning of the
            // buffer
            for(; jj < hig && jj >= 0; jj+=inc) {
                memcpy(
                    outputBuffer + (jj*wid) * kBytesPerPixel,
//scroll_buffer + (jj*wid * kBytesPerPixel),
                    inputBuffer + (
                        (scrolled_shared_rect.top()+jj)*dest_texture_width
                        + scrolled_shared_rect.left()) * kBytesPerPixel,
                    wid*kBytesPerPixel
                );
            }

            // And finally, we push it back into the texture in the right
            // location
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                shared_rect.left(), shared_rect.top(),
                shared_rect.width(), shared_rect.height(),
                GL_BGRA, GL_UNSIGNED_BYTE, outputBuffer
            );
        }
    }

    if (DEBUG_PAINT) {
      /*      std::cout << (void*)wini << " Bitmap rect: w="
      //              << bitmap_rect.width()<<", h="<<bitmap_rect.height()
      //               <<", ("<<bitmap_rect.top()<<","<<bitmap_rect.left()
      //               <<") tex size "<<dest_texture_width<<"x"<<dest_texture_height
                    <<std::endl;
      */
    }
    for (size_t i = 0; i < num_copy_rects; i++) {
        int wid = copy_rects[i].width();
        int hig = copy_rects[i].height();
        int top = copy_rects[i].top() - bitmap_rect.top();
        int left = copy_rects[i].left() - bitmap_rect.left();
        if (DEBUG_PAINT) {
	  /*          std::cout << (void*)wini << " Copy rect: w=" << wid << ", h=" << hig << ", ("
                      << top << "," << left << ")" << std::endl;
	  */
        }
        for(int jj = 0; jj < hig; jj++) {
            memcpy(
                scroll_buffer + jj*wid*kBytesPerPixel,
                bitmap_in + (left + (jj+top)*bitmap_rect.width())*kBytesPerPixel,
                wid*kBytesPerPixel
                );
        }

        // Finally, we perform the main update, just copying the rect that is
        // marked as dirty but not from scrolled data.
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        copy_rects[i].left(), copy_rects[i].top(),
                        wid, hig,
                        GL_BGRA, GL_UNSIGNED_BYTE, scroll_buffer
            );
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}


/** Maps an input coordinate to a texture coordinate for injection into
 *  Berkelium.
 /*
unsigned int mapGLUTCoordToTexCoord(
    unsigned int glut_coord, unsigned int glut_size,
    unsigned int tex_size) {

    return (glut_coord * tex_size) / glut_size;
}
*/
/** Given modifiers retrieved from GLUT (e.g. glutGetModifiers), convert to a
 *  form that can be passed to Berkelium.
 */
/*
int mapGLUTModsToBerkeliumMods(int modifiers) {
    int wvmods = 0;

    if (modifiers & GLUT_ACTIVE_SHIFT)
        wvmods |= Berkelium::SHIFT_MOD;
    if (modifiers & GLUT_ACTIVE_CTRL)
        wvmods |= Berkelium::CONTROL_MOD;
    if (modifiers & GLUT_ACTIVE_ALT)
        wvmods |= Berkelium::ALT_MOD;

    // Note: GLUT doesn't expose Berkelium::META_MOD

    return wvmods;
}
*/


// A few of the most useful keycodes handled below.
enum VirtKeys {
BK_KEYCODE_PRIOR = 0x21,
BK_KEYCODE_NEXT = 0x22,
BK_KEYCODE_END = 0x23,
BK_KEYCODE_HOME = 0x24,
BK_KEYCODE_LEFT = 0x25,
BK_KEYCODE_UP = 0x26,
BK_KEYCODE_RIGHT = 0x27,
BK_KEYCODE_DOWN = 0x28,
BK_KEYCODE_INSERT = 0x2D,
BK_KEYCODE_DELETE = 0x2E

};

/** Given an input key from GLUT, convert it to a form that can be passed to
 *  Berkelium.
 */
/*
unsigned int mapGLUTKeyToBerkeliumKey(int glut_key) {
    switch(glut_key) {
#define MAP_VK(X, Y) case GLUT_KEY_##X: return BK_KEYCODE_##Y;
        MAP_VK(INSERT, INSERT);
        MAP_VK(HOME, HOME);
        MAP_VK(END, END);
        MAP_VK(PAGE_UP, PRIOR);
        MAP_VK(PAGE_DOWN, NEXT);
        MAP_VK(LEFT, LEFT);
        MAP_VK(RIGHT, RIGHT);
        MAP_VK(UP, UP);
        MAP_VK(DOWN, DOWN);
      default: return 0;
    }
}
*/
/** GLTextureWindow handles rendering a window into a GL texture.  Unlike the
 *  utility methods, this takes care of the entire process and cleanup.
 */
//================================================================================================
// GLBerkeliumWindow Class - 
//    this is based on, and borrows heavily from glut_util.hpp in Berkelium's demos directory
//
//  encapsulates functionality from Berkelium's Window and WindowDelegate classes into a single
//  class, to provide a single, simple, interface for loading a url, rendering it to an opengl
//  texture, and displaying it.
//
//
// TODO:
//  currently handles drawing, but should request draw from Graphics system
//  streamline registration of texture
//================================================================================================
class GLBerkeliumWindow : public Berkelium::WindowDelegate 
{
public:
  //==============================================================================================
    GLBerkeliumWindow(unsigned int _w, unsigned int _h, bool _usetrans)
     : width(_w),
       height(_h),
       needs_full_refresh(true),
       m_uiSignal(0)
    {
        // Create texture to hold rendered view
        glGenTextures(1, &web_texture);
        glBindTexture(GL_TEXTURE_2D, web_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        scroll_buffer = new char[width*(height+1)*4];

        Berkelium::Context *context = Berkelium::Context::create();
        bk_window = Berkelium::Window::create(context);
        delete context;
        bk_window->setDelegate(this);
        bk_window->resize(width, height);
        bk_window->setTransparent(_usetrans);
	
	m_uiSignal = new sigc::signal<void,std::string*>();
   }
  //==============================================================================================
  ~GLBerkeliumWindow() {
    delete scroll_buffer;
    delete bk_window;
  }
  //==============================================================================================
  Berkelium::Window* getWindow() {
    return bk_window;
  }
  Berkelium::Window* window() const {
    return bk_window;
  }
  //==============================================================================================
  bool loadURL( const std::string & url )
  {
    return this->bk_window->navigateTo( url.data(), url.length() );
  }
  //==============================================================================================
  void clear() 
  {
        // Black out the page
        unsigned char black = 0;
        glBindTexture(GL_TEXTURE_2D, web_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, 1, 1, 0,
            GL_LUMINANCE, GL_UNSIGNED_BYTE, &black);

        needs_full_refresh = true;
  }
  //==============================================================================================
  // draw function
  //  responsible for drawing this
  unsigned int gldraw() 
  {
    return this->web_texture;
    /*    this->bind();
    glBegin(GL_QUADS);
      glTexCoord2f(0.f, 0.f); glVertex3f(-1.f, -1.f, 0.f);
      glTexCoord2f(0.f, 1.f); glVertex3f(-1.f,  1.f, 0.f);
      glTexCoord2f(1.f, 1.f); glVertex3f( 1.f,  1.f, 0.f);
      glTexCoord2f(1.f, 0.f); glVertex3f( 1.f, -1.f, 0.f);
    glEnd();
    */
    //   this->release();
  }
  //==============================================================================================
    void bind() {
      //       glEnable (GL_BLEND);
	//      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, web_texture);
    }
  //==============================================================================================
    void release() {
      //        glBindTexture(GL_TEXTURE_2D, 0);
    }
  //==============================================================================================
    virtual void onPaint(Berkelium::Window *wini,
        const unsigned char *bitmap_in, const Berkelium::Rect &bitmap_rect,
        size_t num_copy_rects, const Berkelium::Rect *copy_rects,
        int dx, int dy, const Berkelium::Rect &scroll_rect) {

        bool updated = mapOnPaintToTexture(
            wini, bitmap_in, bitmap_rect, num_copy_rects, copy_rects,
            dx, dy, scroll_rect,
            web_texture, width, height, needs_full_refresh, scroll_buffer
        );
        if (updated) {
            needs_full_refresh = false;
	    //  glutPostRedisplay();
        }
    }
  //==============================================================================================
  virtual void onConsoleMessage( Berkelium::Window * win,
				 Berkelium::WideString message,
				 Berkelium::WideString sourceId,
				 int line_no) 
  {
    printf("console.log: %ls\n", message.data() );
  }
  //==============================================================================================
  virtual void onExternalHost(
        Berkelium::Window *win,
        Berkelium::WideString message,
        Berkelium::URLString origin,
        Berkelium::URLString target)
  {
    //someone thought it would be a good idea to use wchar_t. It wasn't.
    //apparently it's chromium's fault
    char * cstr = new char[ message.length()+1 ];
    std::string * msg = 0;
    const wchar_t * wmsg = message.data();
    //we only expect ascii characters, this assupmtion will get us into trouble eventually
    for( int i = 0; i < message.length(); i++) {
      cstr[i] = (char)wmsg[i];
    }
    msg = new std::string( cstr, message.length() ); //
    this->m_uiSignal->emit( msg  );
  }
  //==============================================================================================
  //
  void uiConnectSignal( void (*func_ptr)(std::string*)  ){
    this->m_uiSignal->connect( sigc::ptr_fun( func_ptr ) );
  }
  //==============================================================================================

  //==============================================================================================
private:
  // The Berkelium window, i.e. our web page
  Berkelium::Window* bk_window;
  // Width and height of our window.
  unsigned int width, height;
  // Storage for a texture
  unsigned int web_texture;
  // Bool indicating when we need to refresh the entire image
  bool needs_full_refresh;
  // Buffer used to store data for scrolling
  char* scroll_buffer;

  //==============================================================================================
  //callback signals 
  sigc::signal<void,std::string*> * m_uiSignal; 
  //==============================================================================================
};

#endif //_BERKELIUM_GLUT_UTIL_HPP_
