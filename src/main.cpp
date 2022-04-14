#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <limits>

//-----------------------------------------------------------------------------//
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"// for Event
#include "ftxui/component/mouse.hpp"// for Mouse, Mouse::Left, Mouse::Middle, Mouse::None, Mouse::Pressed, Mouse::Released, Mouse::Right, Mouse::WheelDown, Mouse::WheelUp
#include <docopt/docopt.h>
#include <ftxui/component/captured_mouse.hpp>// for ftxui
#include <ftxui/component/component.hpp>// for Slider
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>// for ScreenInteractive
#include <spdlog/spdlog.h>// for logging
#include "buttonbig.h"

// This file will be generated automatically when you run the CMake configuration step.
// It creates a namespace called `cpp_best_practices_game_jam_one`.
// You can modify the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>

//-----------------------------------------------------------------------------//
using namespace ftxui;

namespace Board {
enum Size : size_t {
    Size_2x2 = 2,
    Size_3x3 = 3,
    Size_4x4 = 4,
    Size_5x5 = 5,
    Size_6x6 = 6,
    Size_7x7 = 7,
    Size_8x8 = 8,
    Size_9x9 = 9,
};
}

//-----------------------------------------------------------------------------//
template<std::size_t Width, std::size_t Height>
struct GameBoard {
    static constexpr auto width = Width;
    static constexpr auto height = Height;
    struct Point {
        std::size_t x, y;
        Point operator+ ( const Point &o ) const {
            return { x + o.x, y + o.y };
        }
        Point operator- ( const Point &o ) const {
            return { x - o.x, y - o.y };
        }
    };

    std::array<bool, height * width> values{};
    std::array<bool, height * width> hints_{};
    std::size_t move_count{ 0 };

    bool &operator[] ( const Point &p ) {
        return values.at ( p.x + p.y * width );
    }

    bool operator[] ( const Point &p ) const {
        return values.at ( p.x + p.y * width );
    }

    void visit ( auto visitor ) {
        for ( auto x = 0UL; x < width; x++ ) {
            for ( auto y = 0UL; y < height; y++ ) {
                const Point p{ x, y };
                visitor ( p, *this );
            }
        }
    }

    void visit ( auto visitor ) const {
        for ( auto x = 0UL; x < width; x++ ) {
            for ( auto y = 0UL; y < height; y++ ) {
                const Point p{ x, y };
                if ( visitor ( p, *this ) ) {
                    break;
                }
            }
        }
    }

    void clear ( ) {
        visit ( [ = ] ( const auto & p, auto & gameboard ) {
            gameboard[p] = true;
            gameboard.hints ( p ) = false;
        } );
    }

    bool& hints ( const Point& p ) {
        return hints_.at ( p.x + p.y * width );
    }

    GameBoard() {
        clear();
    }

    /// Toggle one LED
    void toggle ( const Point &p ) {
        if ( p.x >= width ) {
            return;
        }
        if ( p.y >= height ) {
            return;
        }
        ( *this ) [p] ^= true;
    }

    /// Togle cross (5 LEDs)
    void press ( const Point &p ) {
        ++move_count;
        hints_.at ( p.x + p.y * width ) ^= true;
        toggle ( p );
        toggle ( p - Point{ 1, 0 } );
        toggle ( p - Point{ 0, 1 } );
        toggle ( p + Point{ 1, 0 } );
        toggle ( p + Point{ 0, 1 } );
    }

    [[nodiscard]] bool solved() const {
        bool one = ( *this ) [ { 1, 1 }];
        auto result = true;
        visit ( [&] ( const auto & p, const auto & gb ) -> bool {
            if ( one != gb[p] ) {
                result = false;
                return true;// break
            }
            return false;// go on
        } );

        return result;
    }
};

//-----------------------------------------------------------------------------//
struct Bools {
    bool *state;
    bool* hint;
    bool* show_hints;
};

//-----------------------------------------------------------------------------//
class LEDBase : public ComponentBase {
public:
    LEDBase ( const Bools& b, std::function<void() > on_click ) :
        state_ ( b.state ), hint_ ( b.hint ), show_hints_ ( b.show_hints ), hovered_ ( false ), on_change ( std::move ( on_click ) ) {}

private:
    // Component implementation.
    Element Render() override {
        // bool is_focused = Focused();
        // bool is_active = Active();

        static auto dp = [] ( const Color & c, const Color & bgc ) {
            // return text ( "▄" ) | color ( c ) | bgcolor ( bgc );
            return text ( "▀" ) | color ( c ) | bgcolor ( bgc );
        };

        static constexpr std::array<uint8_t, std::size_t ( 8 * 8 ) > pattern_on {
            0, 10, 20, 20, 20, 20, 10, 0,
            10, 20, 89, 89, 89, 75, 20, 10,
            20, 89, 99, 99, 89, 89, 75, 20,
            20, 89, 99, 99, 89, 89, 75, 20,
            20, 89, 89, 89, 89, 89, 75, 20,
            20, 75, 89, 89, 89, 75, 75, 20,
            10, 20, 75, 75, 75, 75, 20, 10,
            0, 10, 20, 20, 20, 20, 10, 0,
        };

        static constexpr std::array<uint8_t, std::size_t ( 8 * 8 ) > pattern_off {
            0,  0,  0,  0,  0,  0, 0,  0,
            0,  0, 30, 30, 30, 10, 0,  0,
            0, 30, 36, 36, 30, 30, 10, 0,
            0, 30, 36, 36, 30, 30, 10, 0,
            0, 30, 30, 30, 30, 30, 10, 0,
            0, 10, 30, 30, 30, 10, 10, 0,
            0,  0, 10, 10, 10, 10,  0, 0,
            0,  0,  0,  0,  0,  0,  0, 0,
        };

        static const double T = 0.3;

        std::chrono::time_point now = std::chrono::steady_clock::now();
        std::chrono::duration<double> d = now - old_;
        old_ = now;
        startup_ = *state_ ? fmin ( T, startup_ + d.count() ) : fmax ( 0.0, startup_ - d.count() );
        auto r = startup_ / T;

        auto c = [&] ( std::size_t i ) {
            auto v = uint8_t ( r * pattern_on.at ( i ) + ( 1.0 - r ) * pattern_off.at ( i ) );
            v = uint8_t ( v * std::numeric_limits<uint8_t>::max() / 100 );    // NOLINT magic numbers
            if ( show_hints_ != nullptr && *show_hints_ && hint_ != nullptr && *hint_ ) {
                return Color ( v, v * uint8_t ( hovered_ ), 0 );
            }
            return Color ( v * uint8_t ( !hovered_ ), v, v * uint8_t ( !hovered_ ) );
        };

        auto line = [&] ( std::size_t l ) {
            return hbox ( {
                dp ( c ( l * 8 + 0 ), c ( l * 8 + 8 ) ),// NOLINT magic numbers
                dp ( c ( l * 8 + 1 ), c ( l * 8 + 9 ) ),// NOLINT magic numbers
                dp ( c ( l * 8 + 2 ), c ( l * 8 + 10 ) ),// NOLINT magic numbers
                dp ( c ( l * 8 + 3 ), c ( l * 8 + 11 ) ),// NOLINT magic numbers
                dp ( c ( l * 8 + 4 ), c ( l * 8 + 12 ) ),// NOLINT magic numbers
                dp ( c ( l * 8 + 5 ), c ( l * 8 + 13 ) ),// NOLINT magic numbers
                dp ( c ( l * 8 + 6 ), c ( l * 8 + 14 ) ),// NOLINT magic numbers
                dp ( c ( l * 8 + 7 ), c ( l * 8 + 15 ) ),// NOLINT magic numbers
            } );
        };

        auto led = [&]() {
            return vbox ( {
                line ( 0 ),// NOLINT magic numbers
                line ( 2 ),// NOLINT magic numbers
                line ( 4 ),// NOLINT magic numbers
                line ( 6 ),// NOLINT magic numbers
            } );
        };

        return led() | reflect ( box_ );
    }

    bool OnEvent ( Event event ) override {
        if ( !CaptureMouse ( event ) ) {
            return false;
        }

        if ( event.is_mouse() ) {
            return OnMouseEvent ( event );
        }

        return false;
    }

    bool OnMouseEvent ( Event event ) {
        hovered_ = box_.Contain ( event.mouse().x, event.mouse().y );

        if ( !CaptureMouse ( event ) ) {
            return false;
        }

        if ( !hovered_ ) {
            before_pressed_ = false;
            return false;
        }

        if ( event.mouse().button == Mouse::Left ) {
            if ( event.mouse().motion == Mouse::Pressed && !before_pressed_ ) {
                before_pressed_ = true;
                on_change();
                return true;
            }
            if ( event.mouse().motion == Mouse::Released ) {
                before_pressed_ = false;
                return true;
            }
        }

        return false;
    }

    [[nodiscard]]
    bool Focusable() const final {
        return false;
    }

    bool *state_ = nullptr;
    bool *hint_ = nullptr;
    bool *show_hints_ = nullptr;
    bool hovered_;
    Box box_;
    std::function<void() > on_change;
    bool before_pressed_ = false;
    double startup_ = 0;
    std::chrono::time_point<std::chrono::steady_clock> old_;
};

//-----------------------------------------------------------------------------//
//Component LED ( bool *b, bool* hint, bool* show_hints, const std::function<void() >& on_click ) {
Component LED ( const Bools& bools, const std::function<void() >& on_click ) {
    return Make<LEDBase> ( bools, on_click );
}

//-----------------------------------------------------------------------------//
template<size_t L>
void game ( const auto& header, const auto& footer, uint32_t random_seed ) {
    auto screen = ScreenInteractive::Fullscreen();

    bool show_hints = false;
    GameBoard<L, L> gb;

    static constexpr auto randomization_iterations = 100;

    std::string moves_text;
    std::string debug_text;
    std::string show_hints_str{"  Show hints  "};
    std::string hide_hints_str{"  Hide hints  "};
    std::string show_hints_text{show_hints_str};

    const auto update_moves_text = [&moves_text] ( const auto & game_board ) {
        moves_text = fmt::format ( "Moves: {}", game_board.move_count );
        if ( game_board.solved() ) {
            moves_text += " Solved!";
        }
    };

    auto reset_game = [&] ( uint32_t rnd_seed ) {
        gb.clear();
        std::mt19937 gen32{ rnd_seed };
        std::uniform_int_distribution<std::size_t> x ( 0, gb.width - 1 );
        std::uniform_int_distribution<std::size_t> y ( 0, gb.height - 1 );
        for ( int i = 0; i < randomization_iterations; i++ ) {
            gb.press ( { x ( gen32 ), y ( gen32 ) } );
        }
        if ( gb.solved() ) {
            gb.press ( { x ( gen32 ), y ( gen32 ) } );
        }
        gb.move_count = 0;
        update_moves_text ( gb );
    };

    const auto make_leds = [&] {
        std::vector<Component> leds;
        leds.reserve ( gb.width * gb.height );
        gb.visit ( [&] ( const auto & p, auto & gbo ) {
            const Bools b{&gbo[p], &gbo.hints ( p ), &show_hints};
            leds.push_back ( LED ( b, [ =, &gbo] {
                if ( !gbo.solved() ) {
                    gbo.press ( p );
                }
                update_moves_text ( gbo );
            } ) );
        } );
        return leds;
    };

    auto leds = make_leds();
    auto back_button = Button ( "  Back  ", screen.ExitLoopClosure() );
    auto hints_button = Button ( &show_hints_text, [&]() {
        show_hints = !show_hints;
        show_hints_text = show_hints
                          ? hide_hints_str
                          : show_hints_str;
    } );
    auto reset_button = Button ( "  Reset  ", [&]() {
        reset_game ( random_seed );
    } );
    auto new_button = Button ( "  New  ", [&]() {
        random_seed = uint32_t ( std::chrono::high_resolution_clock::now().time_since_epoch().count() );
        reset_game ( random_seed );
    } );

    auto make_layout = [&] {
        std::vector<Element> rows;
        rows.reserve ( gb.width );
        for ( auto x = 0UL; x < gb.width; x++ ) {
            std::vector<Element> row;
            row.reserve ( gb.height );
            for ( auto y = 0UL; y < gb.height; y++ ) {
                row.push_back ( leds[y * gb.width + x]->Render() );
            }
            rows.push_back ( hbox ( std::move ( row ) ) );
        }

        return vbox ( {
            header,
            filler(),
            hbox ( { filler(), text ( moves_text ), filler() } ),
            hbox ( { filler(), vbox ( std::move ( rows ) ) | border, filler() } ),
            filler(),
            hbox ( {
                back_button->Render(),
                filler(),
                hints_button->Render(), reset_button->Render(), new_button->Render(),
                filler(), hbox() | size ( WIDTH, EQUAL, 10 ) } ),// NOLINT magic numbers
            footer,
        } );
    };

    reset_game ( random_seed );
    auto all_buttons = leds;
    all_buttons.push_back ( back_button );
    all_buttons.push_back ( hints_button );
    all_buttons.push_back ( reset_button );
    all_buttons.push_back ( new_button );
    auto container = Container::Horizontal ( all_buttons );
    auto renderer = Renderer ( container, make_layout );

    std::atomic<bool> refresh_ui_continue = true;
    std::thread refresh_ui ( [&] {
        while ( refresh_ui_continue ) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for ( 0.02s ); // NOLINT magic numbers
            screen.PostEvent ( Event::Custom );
        }
    } );

    screen.Loop ( renderer );
    refresh_ui_continue = false;
    refresh_ui.join();
}

//-----------------------------------------------------------------------------//
void boardsize ( const auto& header, const auto& footer, uint32_t random_seed ) {
    auto screen = ScreenInteractive::Fullscreen();

    std::string s2x2_text{ "2x2" };
    std::string s3x3_text{ "3x3" };
    std::string s4x4_text{ "4x4" };
    std::string s5x5_text{ "5x5" };
    std::string s6x6_text{ "6x6" };
    std::string s7x7_text{ "7x7" };
    std::string s8x8_text{ "8x8" };
    std::string s9x9_text{ "9x9" };
    std::string start_text{ "  START  " };
    std::string back_text{ "  BACK  " };

    auto start_button = Button ( &start_text, [&]() {
        game<Board::Size_9x9> ( header, footer, random_seed );
    } );
    auto start2_button = ButtonBig ( &s2x2_text, [&]() {
        game<Board::Size_2x2> ( header, footer, random_seed );
    } );
    auto start3_button = ButtonBig ( &s3x3_text, [&]() {
        game<Board::Size_3x3> ( header, footer, random_seed );
    } );
    auto start4_button = ButtonBig ( &s4x4_text, [&]() {
        game<Board::Size_4x4> ( header, footer, random_seed );
    } );
    auto start5_button = ButtonBig ( &s5x5_text, [&]() {
        game<Board::Size_5x5> ( header, footer, random_seed );
    } );
    auto start6_button = ButtonBig ( &s6x6_text, [&]() {
        game<Board::Size_6x6> ( header, footer, random_seed );
    } );
    auto start7_button = ButtonBig ( &s7x7_text, [&]() {
        game<Board::Size_7x7> ( header, footer, random_seed );
    } );
    auto start8_button = ButtonBig ( &s8x8_text, [&]() {
        game<Board::Size_8x8> ( header, footer, random_seed );
    } );
    auto start9_button = ButtonBig ( &s9x9_text, [&]() {
        game<Board::Size_9x9> ( header, footer, random_seed );
    } );
    auto back_button = Button ( &back_text, screen.ExitLoopClosure() );

    auto b = size ( HEIGHT, EQUAL, 5 ) | size ( WIDTH, EQUAL, 9 ); // NOLINT magic number
    std::vector<Component> buttons;
    auto make_layout = [&] {
        return vbox ( {
            header,
            filler(),
            hbox ( { filler(), text ( "Select board size" ), filler() } ),
            hbox ( {
                filler(), start2_button->Render() | b,
                start3_button->Render() | b,
                start4_button->Render() | b,
                filler() } ),
            hbox ( {filler(), start5_button->Render() | b, start6_button->Render() | b, start7_button->Render() | b, filler() } ),
            hbox ( {filler(), start8_button->Render() | b, start9_button->Render() | b, filler() } ),
            filler(),
            hbox ( { back_button->Render(), filler(), start_button->Render() } ),
            footer,
        } );
    };

    buttons.push_back ( start_button );
    buttons.push_back ( start2_button );
    buttons.push_back ( start3_button );
    buttons.push_back ( start4_button );
    buttons.push_back ( start5_button );
    buttons.push_back ( start6_button );
    buttons.push_back ( start7_button );
    buttons.push_back ( start8_button );
    buttons.push_back ( start9_button );
    buttons.push_back ( back_button );
    auto container = Container::Horizontal ( buttons );
    auto renderer = Renderer ( container, make_layout );

    screen.Loop ( renderer );
}

//-----------------------------------------------------------------------------//
void seed ( const auto& header, const auto& footer ) {
    auto screen = ScreenInteractive::Fullscreen();

    std::string start_text{ " START " };
    std::string back_text{ "  BACK  " };
    std::string input_text{};

    auto start_button = Button ( &start_text, [&]() {
        auto random_seed = uint32_t ( std::hash<std::string> {} ( input_text ) );
        boardsize ( header, footer, random_seed );
    } );
    auto back_button = Button ( &back_text, screen.ExitLoopClosure() );
    auto seed_input = Input ( &input_text, "SEED" );
    std::vector<Component> buttons;
    auto make_layout = [&] {
        return vbox ( {
            header,
            filler(),
            hbox ( {
                seed_input->Render() | border,
            } ),
            filler(),
            hbox ( { back_button->Render(), filler(), start_button->Render() } ),
            footer,
        } );
    };

    buttons.push_back ( start_button );
    buttons.push_back ( seed_input );
    buttons.push_back ( back_button );
    auto container = Container::Horizontal ( buttons );
    auto renderer = Renderer ( container, make_layout );

    screen.Loop ( renderer );
}


//-----------------------------------------------------------------------------//
void menu ( const auto& header, const auto& footer ) {
    auto screen = ScreenInteractive::Fullscreen();

    std::string start_text{ "   START   " };
    std::string seed_text{ "   SEED    " };
    std::string quit_text{ "   QUIT    " };

    auto start_button = Button ( &start_text, [header, footer]() {
        auto random_seed = uint32_t ( std::chrono::high_resolution_clock::now().time_since_epoch().count() );
        boardsize ( header, footer, random_seed );
    } );

    auto seed_button = Button ( &seed_text, [header, footer]() {
        seed ( header, footer );
    } );
    auto quit_button = Button ( &quit_text, screen.ExitLoopClosure() );

    std::vector<Component> buttons;
    auto make_layout = [&] {
        return vbox ( {
            header,
            filler(),
            hbox ( { filler(), start_button->Render(), filler() } ),
            hbox ( { filler(), seed_button->Render(), filler() } ),
            hbox ( { filler(), quit_button->Render(), filler() } ),
            filler(),
            footer,
        } );
    };

    buttons.push_back ( start_button );
    buttons.push_back ( seed_button );
    buttons.push_back ( quit_button );
    auto container = Container::Horizontal ( buttons );
    auto renderer = Renderer ( container, make_layout );

    screen.Loop ( renderer );
}

//-----------------------------------------------------------------------------//
struct ColorRGB {
    std::uint8_t R, G, B;
};

//-----------------------------------------------------------------------------//
// A simple way of representing a bitmap on screen using only characters
struct Bitmap : Node {
    Bitmap ( std::size_t width, std::size_t height ) // NOLINT same typed parameters adjacent to each other
        : width_ ( width ), height_ ( height )
    {}

    ColorRGB &at ( std::size_t x, std::size_t y ) {
        return pixels.at ( width_ * y + x );
    }

    void ComputeRequirement() override
    {
        requirement_ = Requirement{
            .min_x = static_cast<int> ( width_ ), .min_y = static_cast<int> ( height_ / 2 ), .selected_box{ 0, 0, 0, 0 }
        };
    }

    void SetBox ( Box box ) override {
        box_ = box;
    }

    void Render ( Screen &screen ) override
    {
        for ( auto x = 0ULL; x < width_; x++ ) {
            for ( auto y = 0ULL; y < height_; y++ ) {
                auto &p = screen.PixelAt ( box_.x_min + static_cast<int> ( x ), box_.y_min + static_cast<int> ( y ) );
                p.character = "▄";// "▀"
                const auto &top_color = at ( x, y * 2 );
                const auto &bottom_color = at ( x, y * 2 + 1 );
                p.background_color = Color{ top_color.R, top_color.G, top_color.B };
                p.foreground_color = Color{ bottom_color.R, bottom_color.G, bottom_color.B };
            }
        }
    }

    [[nodiscard]] auto width() const noexcept {
        return width_;
    }
    [[nodiscard]] auto height() const noexcept {
        return height_;
    }
    [[nodiscard]] auto &data() noexcept {
        return pixels;
    }

private:
    std::size_t width_;
    std::size_t height_;
    std::vector<ColorRGB> pixels = std::vector<ColorRGB> ( width_ * height_, ColorRGB{} );
};

//-----------------------------------------------------------------------------//
int main ( int argc, const char **argv ) {
    try {
        static constexpr auto USAGE =
            R"(lightsround

 Usage:
       lightsround
       lightsround (-h | --help)
       lightsround --version
 Options:
       -h --help     Show this screen.
       --version     Show version.
        )";

        std::map<std::string, docopt::value> args = docopt::docopt ( USAGE,
        { std::next ( argv ), std::next ( argv, argc ) },
        true,// show help if requested
        fmt::format ( "{} {}.{}.{}",
                      cpp_best_practices_game_jam_one::cmake::project_name,
                      cpp_best_practices_game_jam_one::cmake::project_version_major,
                      cpp_best_practices_game_jam_one::cmake::project_version_minor,
                      cpp_best_practices_game_jam_one::cmake::project_version_patch ) );
        // from config.hpp via CMake
        const auto header = hbox ( {
            text ( fmt::format ( "LightsRound v.{}.{}.{}",
                                 cpp_best_practices_game_jam_one::cmake::project_version_major,
                                 cpp_best_practices_game_jam_one::cmake::project_version_minor,
                                 cpp_best_practices_game_jam_one::cmake::project_version_patch ) ),
            filler(),
            text ( "Seweryn Kamiński" ),
        } );

        const auto footer = hbox ( {
            text ( "2022 - Cpp best practices" ), filler(), text ( "Game Jam 1" )
        } );

        menu ( header, footer );
    } catch ( const std::exception &e ) {
        fmt::print ( "Unhandled exception in main: {}", e.what() );
    }
}

//-----------------------------------------------------------------------------//
