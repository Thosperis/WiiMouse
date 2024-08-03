#include <stdio.h>
#include <windows.h>
#include <wiiuse.h>

#define MAX_WIIMOTES 1
#define SENSITIVITY_STEP 1.0  // Adjust this value to control sensitivity increments
#define MIN_SENSITIVITY 1.0   // Minimum sensitivity
#define MAX_SENSITIVITY 10.0  // Maximum sensitivity
#define D_PAD_SENSITIVITY_STEP 1.0 // Step size for D-pad sensitivity adjustment
#define MIN_D_PAD_SENSITIVITY 1.0 // Minimum D-pad sensitivity
#define MAX_D_PAD_SENSITIVITY 20.0 // Maximum D-pad sensitivity
#define SENSITIVITY 2.0  // Initial sensitivity
#define SMOOTHING_FACTOR 5  // Number of samples to average for stabilization

// Define WIIMOTE_BUTTON_POWER if not defined in the library
#ifndef WIIMOTE_BUTTON_POWER
#define WIIMOTE_BUTTON_POWER 0x1000
#endif

double sensitivity = SENSITIVITY;  // Initial sensitivity
double d_pad_sensitivity = 5.0; // Initial D-pad sensitivity
int pause_pointer = 0;
int adjust_sensitivity_mode = 0;

typedef struct {
    int x;
    int y;
} Point;

Point smooth_buffer[SMOOTHING_FACTOR];
int smooth_index = 0;

void add_to_smooth_buffer(int x, int y) {
    smooth_buffer[smooth_index].x = x;
    smooth_buffer[smooth_index].y = y;
    smooth_index = (smooth_index + 1) % SMOOTHING_FACTOR;
}

Point get_smoothed_point() {
    int sum_x = 0, sum_y = 0;
    for (int i = 0; i < SMOOTHING_FACTOR; ++i) {
        sum_x += smooth_buffer[i].x;
        sum_y += smooth_buffer[i].y;
    }
    Point avg;
    avg.x = sum_x / SMOOTHING_FACTOR;
    avg.y = sum_y / SMOOTHING_FACTOR;
    return avg;
}

void simulate_mouse_click(int button) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = button;
    SendInput(1, &input, sizeof(INPUT));
    input.mi.dwFlags = button == MOUSEEVENTF_LEFTDOWN ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void simulate_mouse_down() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void simulate_mouse_up() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void simulate_key_press(WORD key) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    SendInput(1, &input, sizeof(INPUT));
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void simulate_windows_key_press() {
    INPUT inputs[2] = {0};

    // Press the Windows key
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;

    // Release the Windows key
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_LWIN;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, inputs, sizeof(INPUT));
}

void simulate_mouse_move(int x, int y) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)(x * (65535.0 / GetSystemMetrics(SM_CXSCREEN)));
    input.mi.dy = (LONG)(y * (65535.0 / GetSystemMetrics(SM_CYSCREEN)));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));
}

void move_mouse_relative(int dx, int dy) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)(dx * d_pad_sensitivity);
    input.mi.dy = (LONG)(dy * d_pad_sensitivity);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
}

void reset_mouse_position() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)(32767);  // Center of the screen (65535 / 2)
    input.mi.dy = (LONG)(32767);  // Center of the screen (65535 / 2)
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));
}

void handle_wiimote_event(struct wiimote_t* wm) {
    static int a_button_down = 0;
    static int b_button_pressed = 0;

    // Adjust sensitivity
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_ONE)) {
        sensitivity += SENSITIVITY_STEP;
        if (sensitivity > MAX_SENSITIVITY) sensitivity = MAX_SENSITIVITY;
        printf("Sensitivity increased to: %.1f\n", sensitivity);
        adjust_sensitivity_mode = 1;
        return;  // Prevent further processing
    }

    if (IS_PRESSED(wm, WIIMOTE_BUTTON_TWO)) {
        sensitivity -= SENSITIVITY_STEP;
        if (sensitivity < MIN_SENSITIVITY) sensitivity = MIN_SENSITIVITY;
        printf("Sensitivity decreased to: %.1f\n", sensitivity);
        adjust_sensitivity_mode = 1;
        return;  // Prevent further processing
    }

    // Adjust D-pad sensitivity with + and -
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_MINUS)) {
        d_pad_sensitivity -= D_PAD_SENSITIVITY_STEP;
        if (d_pad_sensitivity < MIN_D_PAD_SENSITIVITY) d_pad_sensitivity = MIN_D_PAD_SENSITIVITY;
        printf("D-pad Sensitivity decreased to: %.1f\n", d_pad_sensitivity);
        return;  // Prevent further processing
    }

    if (IS_PRESSED(wm, WIIMOTE_BUTTON_PLUS)) {
        d_pad_sensitivity += D_PAD_SENSITIVITY_STEP;
        if (d_pad_sensitivity > MAX_D_PAD_SENSITIVITY) d_pad_sensitivity = MAX_D_PAD_SENSITIVITY;
        printf("D-pad Sensitivity increased to: %.1f\n", d_pad_sensitivity);
        return;  // Prevent further processing
    }

    // Pause pointer movement with Home button
    if (IS_HELD(wm, WIIMOTE_BUTTON_HOME)) {
        pause_pointer = 1;
    } else {
        pause_pointer = 0;
    }

    // Handle D-pad for mouse movement
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_UP)) {
        move_mouse_relative(0, -1);
    }
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_DOWN)) {
        move_mouse_relative(0, 1);
    }
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_LEFT)) {
        move_mouse_relative(-1, 0);
    }
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_RIGHT)) {
        move_mouse_relative(1, 0);
    }

    // Handle button presses
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_A)) {
        if (!a_button_down) {
            simulate_mouse_down();
            a_button_down = 1;
        }
    } else {
        if (a_button_down) {
            simulate_mouse_up();
            a_button_down = 0;
        }
    }

    if (IS_PRESSED(wm, WIIMOTE_BUTTON_B)) {
        if (!b_button_pressed) {
            simulate_mouse_click(MOUSEEVENTF_RIGHTDOWN);
            b_button_pressed = 1;
        }
    } else {
        if (b_button_pressed) {
            simulate_mouse_click(MOUSEEVENTF_RIGHTUP);
            b_button_pressed = 0;
        }
    }

    // Handle power button to press Windows key
    if (IS_PRESSED(wm, WIIMOTE_BUTTON_POWER)) {
        simulate_windows_key_press();
    }

    // Handle IR data for pointer movement
    if (WIIUSE_USING_IR(wm) && !adjust_sensitivity_mode && !pause_pointer) {
        int x = wm->ir.dot[0].x;
        int y = wm->ir.dot[0].y;
        if (wm->ir.dot[0].visible) {
            add_to_smooth_buffer(x, y);
            Point smoothed_point = get_smoothed_point();
            simulate_mouse_move(smoothed_point.x * sensitivity, smoothed_point.y * sensitivity);
        }
    }

    // Reset the flag
    adjust_sensitivity_mode = 0;
}

int main() {
    wiimote** wiimotes;
    int found, connected;

    wiimotes = wiiuse_init(MAX_WIIMOTES);

    found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
    if (!found) {
        printf("No wiimotes found.\n");
        return 0;
    }

    connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
    if (connected) {
        printf("Connected to %i wiimote(s) (of %i found).\n", connected, found);
    } else {
        printf("Failed to connect to any wiimote.\n");
        return 0;
    }

    // Initialize smooth buffer
    for (int i = 0; i < SMOOTHING_FACTOR; ++i) {
        smooth_buffer[i].x = 0;
        smooth_buffer[i].y = 0;
    }

    // Enable IR and report modes
    for (int i = 0; i < connected; ++i) {
        wiiuse_set_ir(wiimotes[i], 1);
        wiiuse_motion_sensing(wiimotes[i], 1);
    }

    while (1) {
        if (wiiuse_poll(wiimotes, MAX_WIIMOTES)) {
            for (int i = 0; i < MAX_WIIMOTES; ++i) {
                if (wiimotes[i]->event == WIIUSE_EVENT) {
                    handle_wiimote_event(wiimotes[i]);
                }
            }
        }
    }

    wiiuse_cleanup(wiimotes, MAX_WIIMOTES);
    return 0;
}