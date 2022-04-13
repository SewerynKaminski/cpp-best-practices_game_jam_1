#include "buttonbig.h"

ButtonBigBase::ButtonBigBase ( ConstStringRef label,
                               std::function<void() > on_click,
                               Ref<ButtonOption> option ) : label_ ( label ), on_click_ ( on_click ), option_ ( option ) {

}

Element ButtonBigBase::Render() {
    auto style = Focused() ? inverted : nothing;
    auto my_border = option_->border ? border : nothing;
    return vbox ( { filler(),
                    hbox ( { filler(), text ( *label_ ), filler() } ),
                    filler() } ) | my_border | style | reflect ( box_ );
}

bool ButtonBigBase::OnEvent ( Event event ) {
    if ( event.is_mouse() && box_.Contain ( event.mouse().x, event.mouse().y ) ) {
        if ( !CaptureMouse ( event ) )
            return false;

        TakeFocus();

        if ( event.mouse().button == Mouse::Left &&
                event.mouse().motion == Mouse::Pressed ) {
            on_click_();
            return true;
        }

        return false;
    }

    if ( event == Event::Return ) {
        on_click_();
        return true;
    }
    return false;
}


//-----------------------------------------------------------------------------//
Component ButtonBig ( ConstStringRef label,
                      std::function<void() > on_click,
                      Ref<ButtonOption> option ) {
    return Make<ButtonBigBase> ( label, std::move ( on_click ), std::move ( option ) );
}
