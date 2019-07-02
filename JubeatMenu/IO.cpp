#include "IO.h"

IO::IO()
{
	/* Start with not being ready */
	is_ready = false;

	/* Load the log function so we can output debugging */
	log_function = NULL;
	core = GetModuleHandleA("avs2-core.dll");
	if (core != NULL) {
		log_function = (log_func)GetProcAddress(core, "XCgsqzn000017c");
	} else {
		core = GetModuleHandleA("libavs-win32.dll");
		if (core != NULL) {
			log_function = (log_func)GetProcAddress(core, "XCd229cc000018");
		}
	}

	debug("Initializing device.dll");

	device = GetModuleHandleA("device.dll");

	if (device == NULL) {
		debug("Missing device.dll. Did you run this from the right directory?");
		return;
	}

	device_initialize = (INT_RET_ONE_ARG) GetProcAddress(device, "device_initialize");
	device_is_initialized = (INT_RET_NO_ARGS) GetProcAddress(device, "device_is_initialized");
	device_get_status = (INT_RET_NO_ARGS) GetProcAddress(device, "device_get_status");
	device_set_panel_mode = (INT_RET_ONE_ARG) GetProcAddress(device, "device_set_panel_mode");
	device_get_panel_trg_on = (INT_RET_THREE_ARGS) GetProcAddress(device, "device_get_panel_trg_on");
	device_get_panel_trg_off = (INT_RET_THREE_ARGS) GetProcAddress(device, "device_get_panel_trg_off");
	device_get_panel_trg_short_on = (INT_RET_THREE_ARGS) GetProcAddress(device, "device_get_panel_trg_short_on");
	device_update = (INT_RET_NO_ARGS) GetProcAddress(device, "device_update");
	device_finalize = (INT_RET_NO_ARGS) GetProcAddress(device, "device_finalize");

	if (
		device_initialize == NULL ||
		device_is_initialized == NULL ||
		device_get_status == NULL ||
		device_set_panel_mode == NULL ||
		device_update == NULL ||
		device_get_panel_trg_on == NULL ||
		device_get_panel_trg_off == NULL ||
		device_get_panel_trg_short_on == NULL ||
		device_finalize == NULL
	) {
		debug("Couldn't find correct functions to call! Did you use the right DLLs?");
		FreeLibrary(device);
		return;
	}

	// First, initialize and verify.
	debug("Initializing P4IO device");
	device_initialize(0);
	
	if (!device_is_initialized()) {
		debug("Couldn't initialize P4IO device");
		FreeLibrary(device);
		return;
	}
	
	if (device_get_status() < 0) {
		debug("P4IO device status returns error!");
		FreeLibrary(device);
		return;
	}

	// Configure to read panels
	debug("Configuring P4IO driver");
	device_set_panel_mode(0);

	// We're ready to roll
	debug("P4IO driver ready for input");
	is_ready = true;
	buttons = 0;
	lastButtons = 0;
}

IO::~IO(void)
{
	if (!is_ready) { return; }

	debug("Finalizing P4IO driver");
	device_finalize();
	FreeLibrary(device);
}

bool IO::Ready()
{
	return is_ready;
}

void IO::ErrorMessage(char *msg)
{
	debug(msg);
}

void IO::Tick()
{
	if (!is_ready) { return; }

	if(device_is_initialized()) {
		device_update();
	}

	// Remember last buttons so we can calculate newly
	// pressed buttons.
	lastButtons = buttons;

	for (int panel = 0; panel < 16; panel++) {
		int output[2] = { 0 };

		device_get_panel_trg_on(panel % 4, panel / 4, output);
		if (output[0] != 0) {
			// Track the button release
			buttons &= ~(1 << panel);
		}
		
		device_get_panel_trg_off(panel % 4, panel / 4, output);
		if (output[0] != 0) {
			// Track the button press
			buttons |= (1 << panel);
		}
	}
}

unsigned int IO::ButtonsHeld()
{
	return buttons;
}

bool IO::ButtonHeld(unsigned int button)
{
	return (ButtonsHeld() & button) != 0;
}

unsigned int IO::ButtonsPressed()
{
	return buttons & (~lastButtons);
}

bool IO::ButtonPressed(unsigned int button)
{
	return (ButtonsPressed() & button) != 0;
}