#ifndef BUTTONBIG_H
#define BUTTONBIG_H
#include <functional>  // for function
#include <memory>      // for shared_ptr
#include <utility>     // for move

#include "ftxui/component/captured_mouse.hpp"     // for CapturedMouse
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"  // for ButtonOption
#include "ftxui/component/event.hpp"              // for Event, Event::Return
#include "ftxui/component/mouse.hpp"  // for Mouse, Mouse::Left, Mouse::Pressed
#include "ftxui/component/screen_interactive.hpp"  // for Component
#include "ftxui/dom/elements.hpp"  // for operator|, Element, nothing, reflect, text, border, inverted
#include "ftxui/screen/box.hpp"  // for Box
#include "ftxui/util/ref.hpp"    // for ConstStringRef, Ref

using namespace ftxui;

class ButtonBigBase : public ComponentBase {
    public:
        ButtonBigBase ( ConstStringRef label,
                        std::function<void() > on_click,
                        Ref<ButtonOption> option );
    Element Render() override;
    bool OnEvent ( Event event ) override;
    bool Focusable() const final {
        return true;
    }

    private:
        ConstStringRef label_;
        std::function<void() > on_click_;
        Box box_;
        Ref<ButtonOption> option_;
};


//-----------------------------------------------------------------------------//
Component ButtonBig ( ConstStringRef label,
                      std::function<void() > on_click,
                      Ref<ButtonOption> = {} );

#endif // BUTTONBIG_H
