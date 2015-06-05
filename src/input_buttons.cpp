#if !(defined(OPENDINGUX) || defined(GEKKO))
#  include "platform/input_buttons_desktop.cpp"
#endif

#if defined(GEKKO)
#  include "platform/input_buttons_gekko.cpp"
#endif

#if defined(GPH)
#  include "platform/input_buttons_gph.cpp"
#endif

#if defined(PSP)
#  include "platform/input_buttons_psp.cpp"
#endif

#if defined(OPENDINGUX)
#  include "platform/input_buttons_opendingux.cpp"
#endif
