#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <random>

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

// This file will be generated automatically when you run the CMake configuration step.
// It creates a namespace called `cpp_best_practices_game_jam_one`.
// You can modify the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>

//-----------------------------------------------------------------------------//
using namespace ftxui;

//-----------------------------------------------------------------------------//
template<std::size_t Width, std::size_t Height> struct GameBoard {
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
    std::size_t move_count{ 0 };

    std::string &get_string ( std::size_t x, std::size_t y ) {
        return get_string ( { x, y } );
    }

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

    GameBoard() {
        visit ( [ = ] ( const auto & p, auto & gameboard ) {
            gameboard[p] = true;
        } );
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
class LEDBase : public ComponentBase {
public:
    LEDBase ( bool *state, std::function<void() > on_click ) : state_ ( state ), hovered_ ( false ), on_change ( std::move ( on_click ) ) {}

private:
    // Component implementation.
    Element Render() override {
        // bool is_focused = Focused();
        // bool is_active = Active();

        static auto dp = [] ( const Color & c, const Color & bgc ) {
            // return text ( "▄" ) | color ( c ) | bgcolor ( bgc );
            return text ( "▀" ) | color ( c ) | bgcolor ( bgc );
        };

        static constexpr std::array<uint8_t, 8UL * 8UL> pattern_on {
            0, 10, 20, 20, 20, 20, 10, 0,
            10, 20, 89, 89, 89, 75, 20, 10,
            20, 89, 99, 99, 89, 89, 75, 20,
            20, 89, 99, 99, 89, 89, 75, 20,
            20, 89, 89, 89, 89, 89, 75, 20,
            20, 75, 89, 89, 89, 75, 75, 20,
            10, 20, 75, 75, 75, 75, 20, 10,
            0, 10, 20, 20, 20, 20, 10, 0,
        };

        static constexpr std::array<uint8_t, 8UL * 8UL> pattern_off {
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
            v = uint8_t ( v * UCHAR_MAX / 100 );    // NOLINT magic numbers
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
    bool hovered_;
    Box box_;
    std::function<void() > on_change;
    bool before_pressed_ = false;
    double startup_ = 0;
    std::chrono::time_point<std::chrono::steady_clock> old_;
};

//-----------------------------------------------------------------------------//
Component LED ( bool *b, const std::function<void() >& on_click ) {
    return Make<LEDBase> ( b, on_click );
}

//-----------------------------------------------------------------------------//
void game ( const auto& header, const auto& footer, uint64_t random_seed ) {
    auto screen = ScreenInteractive::Fullscreen();

    GameBoard<9, 9> gb;

    std::string moves_text;
    std::string debug_text;

    const auto update_moves_text = [&moves_text] ( const auto & game_board ) {
        moves_text = fmt::format ( "Moves: {}", game_board.move_count );
        if ( game_board.solved() ) {
            moves_text += " Solved!";
        }
    };

    const auto make_leds = [&] {
        std::vector<Component> leds;
        leds.reserve ( gb.width * gb.height );
        gb.visit ( [&] ( const auto & p, auto & gbo ) {
            leds.push_back ( LED ( &gbo[p], [ =, &gbo] {
                if ( !gbo.solved() ) {
                    gbo.press ( p );
                }
                update_moves_text ( gbo );
            } ) );
        } );
        return leds;
    };

    auto leds = make_leds();
    auto quit_button = Button ( "  Back  ", screen.ExitLoopClosure() );
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
            hbox ( quit_button->Render() ),
            footer,
        } );
    };

    static constexpr auto randomization_iterations = 100;

    std::mt19937 gen32{random_seed};
    //gen32.seed ( random_seed );
    std::uniform_int_distribution<std::size_t> x ( 0, gb.width - 1 );
    std::uniform_int_distribution<std::size_t> y ( 0, gb.height - 1 );

    for ( int i = 0; i < randomization_iterations; i++ ) {
        gb.press ( { x ( gen32 ), y ( gen32 ) } );
    }
    gb.move_count = 0;
    update_moves_text ( gb );

    auto all_buttons = leds;
    all_buttons.push_back ( quit_button );
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
void seed ( const auto& header, const auto& footer ) {
    auto screen = ScreenInteractive::Fullscreen();

    std::string start_text{ " START " };
    std::string back_text{ "  BACK  " };
    std::string input_text{};

    auto start_button = Button ( &start_text, [&]() {
        uint64_t random_seed = std::hash<std::string> {} ( input_text );
        game ( header, footer, random_seed );
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
        uint64_t random_seed = uint64_t ( std::chrono::high_resolution_clock::now().time_since_epoch().count() );
        game ( header, footer, random_seed );
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
        for ( auto x = 0UL; x < width_; x++ ) {
            for ( auto y = 0UL; y < height_; y++ ) {
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
