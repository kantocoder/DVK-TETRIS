
#include <chrono>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
#include <GL/freeglut.h>

#include "bitmap_fonts.h"
#include "Position.h"
#include "Figure.h"


const  int g_Well_Depth = 20;
const  int g_Well_Width = 10;
const  GLfloat DVK_COLOR[3] = {160./255,209./255,155./255};


enum class GameMode : int 
{
    NormalFall = 0,
    FreeFall = 1,
    GameOver = 2,
    QuitGame = 3,
    NewGame = 4
};


class Tetris
{
    std::atomic <GameMode>  _game_mode;
    std::atomic <bool>      _is_stop, _is_ctrl_thread_run, _is_redraw;
    std::atomic <int>       _current_fig_index, _next_fig_index, _score, _level;
    std::vector <Figure>    _v_figures;
    std::vector <int>       _v_switch_index, _v_figure_to_switch_idx;

    std::mutex              _well_mutex;
    std::thread             _control_thread;

    int                     _sleep_period_ms;
    int                     _Well[g_Well_Depth][g_Well_Width];
    int                     _win[2];

    Position                _current_position;


    //  OpenGL functions

    static void static_gl_idle()            {   _instance_ptr->gl_idle();       }

    static void static_gl_display()         {   _instance_ptr->gl_display();    }

    static void static_gl_keyboard
        (uint8_t key, int x, int y)         {   _instance_ptr->gl_keyboard(key, x, y);   }

    static void static_gl_keyboard_spec
        (int key, int x, int y)             {   _instance_ptr->gl_keyboard_spec(key, x, y); }

    static void static_gl_reshape 
        (int w, int h)                      {   _instance_ptr->gl_reshape(w,h); }


    void gl_keyboard_spec(int key, int x, int y) 
    {
        Position newpos;

        switch (key) 
        {
            case GLUT_KEY_UP:
                rotate_figure();
            break;

            case GLUT_KEY_LEFT:
                move_figure_left();
            break;

            case GLUT_KEY_RIGHT:
                move_figure_right();
            break;
        }
    }

    void gl_keyboard(uint8_t key, int x, int y) 
    {
        if (_game_mode == GameMode::GameOver)
        {
            switch (key) 
            { 
                case 'N': 
                case 'n': 
                    _game_mode = GameMode::QuitGame;
                    _is_redraw = true;
                break;

                case 'Y': 
                case 'y': 
                    _game_mode = GameMode::NewGame;
                break;
            }
        }
        else
        if (_game_mode == GameMode::NormalFall || _game_mode == GameMode::FreeFall)
        {
            switch (key) 
            { 
                case 'Q': 
                case 'q': 
                case 27:
                    _game_mode = GameMode::QuitGame;
                break;
                
                case '8':
                    rotate_figure();
                break;

                case '7':
                    move_figure_left();
                break;

                case '9':
                    move_figure_right();
                break;
                
                case ' ':
                    _game_mode = GameMode::FreeFall;
                break;
            }
        }
    }

    void gl_reshape(int w, int h) 
    {
        if (_is_stop) {
            throw "FrontEnd :: GLUT loop is finished.\n"; // glutLeaveMainLoop();  // <- causes problems at finalization, all kinds of segfaults
        }

        glViewport(0.0, 0.0, (GLsizei) w, (GLsizei) h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glOrtho(-40*8.0f, w - 40*8.0f, h - 32.f, -32.f, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        _win[0] = w;
        _win[1] = h;
    }

    void gl_idle() 
    {

        if (_game_mode == GameMode::QuitGame)
            _is_stop = true;

        if (_is_stop)   
            throw "Tetris :: GLUT loop is finished.\n"; 

        if (_is_redraw==true) {
            _is_redraw=false;
            glutPostRedisplay();
        }
    }

    void gl_display() 
    {
        glViewport(0.0, 0.0, (GLsizei) _win[0], (GLsizei) _win[1]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        gluOrtho2D(-40*8.0f, _win[0] - 40*8.0f, _win[1] - 32.f, -32.f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();


        glClear(GL_COLOR_BUFFER_BIT);

        const Figure & F = _v_figures[_current_fig_index];
        F.render((_current_position.x-g_Well_Width/2)*16,_current_position.y*16,_win);

        const Figure & NF = _v_figures[_next_fig_index];
        NF.render((g_Well_Width/2+4)*16,(g_Well_Depth/2)*16,_win);

        render_well();

        render_score();

        if (_game_mode == GameMode::GameOver)
        {
            glColor3fv(DVK_COLOR);

            beginRenderText(-40*8.0f, -32, _win[0]-40*8, _win[1]-32);
            renderText((g_Well_Width/2+3)*16,(g_Well_Depth/2+2)*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "New game (Y/N)? ");
            endRenderText();
        }


        glFlush();
        glutSwapBuffers();
    }


    void init_GL() 
    {
        int dummyargc = 0;
        char * dummyargv[1];

        glutInit(&dummyargc, (char**) dummyargv);
        glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
        glutInitWindowSize(_win[0], _win[1]);
        glutInitWindowPosition(0 + 1366 + 100, 0 + 100);
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION );             //
        glutCreateWindow("DVK TETRIS");

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glShadeModel(GL_FLAT);
    }

    void set_GL_functions() 
    {
        glutIdleFunc(static_gl_idle);
        glutDisplayFunc(static_gl_display);
        glutKeyboardFunc(static_gl_keyboard);
        glutSpecialFunc(static_gl_keyboard_spec);
        glutReshapeFunc(static_gl_reshape);
        glutCloseFunc(Tetris::onGlutClose);
    }

    static void onGlutClose() 
    {
        std::cout <<"Quitting DVK Tetris\n";
    }

    
    //  Figure-related operation

    void    rotate_figure()
    {
        int idx = _v_switch_index [_current_fig_index];
        if (check_figure(idx,_current_position)) 
        {
            _current_fig_index = idx;
            _is_redraw = true;
        }
    }

    void    move_figure_left()
    {
        Position newpos (_current_position);
        --newpos.x;

        if (check_figure(_current_fig_index, newpos))
        {
            _current_position = newpos;
            _is_redraw = true;
        }
    }

    void    move_figure_right()
    {
        Position newpos (_current_position);     
        ++newpos.x;

        if (check_figure(_current_fig_index, newpos))
        {
            _current_position = newpos;
            _is_redraw = true;
        }
    }

    bool    check_figure (int index, const Position & curpos)
    {
        const Figure & F = _v_figures[index];

        for (int i = 0; i<4; i++)
        {
            int x = curpos.x + F.x[i];
            int y = curpos.y + F.y[i];
            
            if (x<0 || x > g_Well_Width-1) return false;
            if (y<0 || y > g_Well_Depth-1) return false;   

            {
                std::lock_guard<std::mutex> lock(_well_mutex);

                if (_Well[y][x]!=0)
                    return false;
            }
        }

        return true;
    }

    //  Well-related operations
    
    void    render_well()
    {
        beginRenderText(-40*8, -32, _win[0]-40*8, _win[1]-32);

        for (int y=0;y<g_Well_Depth; ++y)
        {
            glColor3fv(DVK_COLOR);

            renderText(-16-(g_Well_Width/2)*16, y*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "<|");
            renderText(((g_Well_Width+1)/2)*16, y*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "|>");

            for (int x=0;x<g_Well_Width; ++x)
            {
                _well_mutex.lock();
                int z = _Well[y][x];
                _well_mutex.unlock();

                if (z!=0) {
                    const GLfloat * clr   = _v_figures[z-1].clr;
                    glColor3fv(clr);

                    renderText((x-g_Well_Width/2)*16, y*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "[]");
                }
                else  {
                    glColor3fv(DVK_COLOR);
                    renderText((x-g_Well_Width/2)*16, y*16, BITMAP_FONT_TYPE_8_BY_13, (char*) " .");
                }
            }
        }

        glColor3fv(DVK_COLOR);
        renderText(-16-(g_Well_Width/2)*16, g_Well_Depth*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "<|");
        renderText(((g_Well_Width+1)/2)*16, g_Well_Depth*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "|>");
        for (int x=0;x<g_Well_Width;x++)
        {

            renderText((x-g_Well_Width/2)*16, g_Well_Depth*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "==");
            renderText((x-g_Well_Width/2)*16, (g_Well_Depth+1)*16, BITMAP_FONT_TYPE_8_BY_13, (char*) "\\/");
        }
       
        endRenderText();              
    }

    void    process_well()
    {
        for (int y=0;y<g_Well_Depth;y++)
        {
            bool is_remove_line (true);

            {
                std::lock_guard<std::mutex> lock (_well_mutex);
                for (int x=0;x<g_Well_Width;x++) 
                    if (_Well[y][x] == 0) { 
                        is_remove_line = false;
                        break;
                    }
            }

            if (is_remove_line)
            {
                std::lock_guard<std::mutex> lock (_well_mutex);

                for (int x=0;x<g_Well_Width;x++) 
                {
                    for (int yy=y;yy>=1;--yy)  
                        _Well[yy][x] = _Well[yy-1][x];
                    
                    _Well[0][x]=0;
                }
            }
            
            if (is_remove_line)
                _is_redraw = true;

        }
    }

    //  
    void  render_score()
    {
        char score_msg[32];
        sprintf(score_msg, "Score: %d", _score.load());
    
        glColor3fv(DVK_COLOR);

        beginRenderText(-40*8, -32, _win[0]-40*8, _win[1]-32);
        renderText((g_Well_Width/2+3)*16, 4*16, BITMAP_FONT_TYPE_8_BY_13, score_msg);
        endRenderText();
    
    }

    //  Thread that controls the execution of the game

    void control_thread_cb()
    {
        const int               min_sleep_time_ms = 5;
        int                     sleep_time;
        int                     current_figure_num, next_figure_num;

        std::vector <int>       v_figure_score{20,22,21,22,18,20,31};
        std::vector <int>       v_level_fall_delay_ms{1000,900,800,700,600,500,400,300,200,100};

        std::random_device      rd;             //  Needed to obtain a seed for the random number engine
        std::mt19937            gen(rd());      //  Standard mersenee_twister_engine seeded with rd()
        std::uniform_int_distribution<>     
                                unif_dist(0,_v_figure_to_switch_idx.size()-1);
        
        _next_fig_index = _v_figure_to_switch_idx[ unif_dist(gen) ];
        
        _is_stop = false;


        _is_ctrl_thread_run = true;

        
        while (!_is_stop)
        {

            if (_game_mode == GameMode::GameOver)
                continue;

            if (_game_mode == GameMode::NewGame)
            {
                //   clear well
                {   
                    std::lock_guard<std::mutex> lock(_well_mutex);

                    for (int y=0;y<g_Well_Depth; ++y)
                      for (int x=0;x<g_Well_Width; ++x)
                        _Well[y][x] = 0;
                }
                    
                //  setting current figure type (0..6), next figure type (0..6)
                current_figure_num = unif_dist(gen);
                next_figure_num = unif_dist(gen);

                _current_fig_index = _v_figure_to_switch_idx[ current_figure_num ];
                _next_fig_index = _v_figure_to_switch_idx[ next_figure_num ];

                //  set position of the new figure to default (top middle of the well)
                _current_position = {g_Well_Width/2,1};                
                
                //  set game mode to Normal Fall
                _game_mode = GameMode::NormalFall;
                _is_redraw = true;

                continue;
            }



            if (_game_mode == GameMode::NormalFall)
                sleep_time = v_level_fall_delay_ms[_level]; //_sleep_period_ms;
            else
            if (_game_mode == GameMode::FreeFall)
                sleep_time = min_sleep_time_ms;


            for (int i = 0; i<sleep_time/min_sleep_time_ms; ++i) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(min_sleep_time_ms));

                if (_game_mode == GameMode::FreeFall) 
                    break;     //  in free fall, the delay is only min_sleep_time_ms between steps

                if (_game_mode == GameMode::QuitGame) {
                    _is_stop = true;
                    break;
                }
            }


            Position  newpos (_current_position);
            ++newpos.y;

            if (check_figure (_current_fig_index, newpos))
            {
                _current_position = newpos;
                _is_redraw = true;
            }
            else 
            {
                //  update score
                _score += v_figure_score[current_figure_num];
                
                //  Copy figure to well
                const Figure & F = _v_figures [_current_fig_index];

                _well_mutex.lock();
                for (int i = 0; i<4; i++)
                {
                    int x = _current_position.x + F.x[i];
                    int y = _current_position.y + F.y[i];
                    _Well[y][x] = _current_fig_index+1;
                }
                _well_mutex.unlock();

                //  process well to remove full lines
                process_well();

                //  assign next figure to current, then randomly select next figure 
                current_figure_num = next_figure_num;
                next_figure_num = unif_dist(gen);

                _current_fig_index.store(_next_fig_index.load());
                _next_fig_index = _v_figure_to_switch_idx[ next_figure_num ];

                //  set position of the new figure to default (top middle of the well)
                _current_position = {g_Well_Width/2,1};
                
                //  check if the player can continue to play
                if( check_figure(_current_fig_index, _current_position) )
                    _game_mode = GameMode::NormalFall;
                else
                    _game_mode = GameMode::GameOver;

                _is_redraw = true;
            }
        }
        
        std::cout<<"Quitting control_thread_cb\n";
    }


public:

    Tetris()
    :   _win{80*8,25*16}//{(g_Well_Width+14)*16,(g_Well_Depth+10)*16}
    ,   _Well{0}
    ,   _score(0)
    ,   _v_figures { 

            {{-1,0,1,1}, {0,0,0,-1}, {1.0, 1.0, 1.0}},   //      []
            {{0,0,0,-1}, {1,0,-1,-1}, {1.0, 1.0, 1.0}},  //  [][][]
            {{1,0,-1,-1}, {0,0,0, 1}, {1.0, 1.0, 1.0}},  // 
            {{0,0,0,1}, {-1,0, 1,1}, {1.0, 1.0, 1.0}},   // 

            {{1,0,-1,-1}, {0,0,0,-1}, {1.0, 1.0, 1.0}},  //  []
            {{0,0,0,-1}, {-1,0,1,1}, {1.0, 1.0, 1.0}},   //  [][][]
            {{-1,0,1,1}, {0,0,0,1}, {1.0, 1.0, 1.0}},    //
            {{0,0,0,1}, {1,0,-1,-1}, {1.0, 1.0, 1.0}},   //    

            {{-1,0,1,0}, {0,0,0,-1}, {1.0, 1.0, 1.0}},   //    []
            {{0,0,0,-1}, {-1,0,1,0}, {1.0, 1.0, 1.0}},   //  [][][]
            {{-1,0,1,0}, {0,0,0, 1}, {1.0, 1.0, 1.0}},   //  
            {{0,0,0, 1}, {-1,0,1,0}, {1.0, 1.0, 1.0}},   //    

            {{0,0,1,1}, {-1,0,0,1}, {1.0, 1.0, 1.0}},    //  [][]
            {{1,0,0,-1}, {0,0,1,1}, {1.0, 1.0, 1.0}},    //    [][]

            {{0,0,1,1}, {-1,0,0,1}, {1.0, 1.0, 1.0}},    //    [][]
            {{1,0,0,-1}, {0,0,1,1}, {1.0, 1.0, 1.0}},    //  [][]

            {{-1,0,1,2}, {0,0,0,0}, {1.0, 1.0, 1.0}/*{.5, 1.0, 0.5}*/},     //  [][][][]
            {{0,0,0,0}, {-1,0,1,2}, {1.0, 1.0, 1.0}/*{.5, 1.0, 0.5}*/},     //  [][][][]^T

            
            {{0,1,0,1}, {0,0,1,1}, {1.0, 1.0, 1.0}}      //  [][]
                                                         //  [][]
        }
    ,   _v_switch_index {

            1/*0->1*/, 2, 3, 0/*3->0*/, 
            5/*4->5*/, 6, 7, 4/*7->4*/,   
            9/*8->9*/, 10,11, 8/*11->8*/,
            13/*12->13*/, 12/*13->12*/,
            15/*14->15*/, 14/*15->14*/,
            17/*16->17*/, 16/*17->16*/,
            18/*18->18*/

        }
    ,   _v_figure_to_switch_idx {0,4,8,12,14,16,18}
    ,   _current_fig_index (0)
    ,   _is_redraw(false)
    ,   _level(0)
    ,   _sleep_period_ms (1000)
    ,   _game_mode(GameMode::NewGame)
    {
        _instance_ptr = this;
    }


    ~Tetris()
    {
        stop();
        std::cout <<"~Tetris() is finished.\n";
    }

    void run() 
    {
        try {
            _is_ctrl_thread_run = false;
            _control_thread = std::thread(&Tetris::control_thread_cb, this);
            while (!_is_ctrl_thread_run);
            
            init_GL();
            set_GL_functions();
            glutMainLoop();
        }  
        catch (const char * msg) {
            std::cout << msg << "\n";
        }
        catch (...)
        {
            std::cout <<"Some other exception occured\n";
        }
    }

    void stop() 
    {
        _is_stop = true;

        if (_control_thread.joinable())
            _control_thread.join();
    }

private:
    static Tetris * _instance_ptr;
};

Tetris * Tetris::_instance_ptr;

