// BLE Voltage Lab handlebar holder for Waveshare ESP32-C6-LCD-1.47.
// Units are millimetres. Tune the parameters below after the first test fit.

$fn = 72;

board_w = 20.32;
board_h = 36.37;
board_clearance = 0.45;
board_depth = 8.2;
screen_w = 17.2;
screen_h = 32.0;
screen_clearance = 1.0;

hole_dx = 13.28;
hole_dy = 32.00;
m2_clearance = 2.35;
standoff_d = 4.8;

wall = 2.2;
back_thickness = 2.4;
front_thickness = 2.0;
face_rim = 4.0;
face_overhang = 4.0;
wire_hole_w = 7.0;
wire_hole_h = 4.5;
wire_channel_d = 5.0;

bar_d = 22.2;
bar_clearance = 0.6;
clamp_w = 28;
clamp_thickness = 5.0;
clamp_gap = 4.0;
clamp_bolt_d = 4.4;
clamp_nut_flat = 7.2;
clamp_nut_depth = 3.2;

tilt_deg = 12;

case_w = board_w + board_clearance * 2 + wall * 2 + face_overhang * 2;
case_h = board_h + board_clearance * 2 + wall * 2 + face_overhang * 2;
case_d = board_depth + back_thickness + front_thickness;
board_pocket_w = board_w + board_clearance * 2;
board_pocket_h = board_h + board_clearance * 2;

module rounded_box(size, r) {
    hull() {
        for (x = [r, size[0] - r])
            for (y = [r, size[1] - r])
                translate([x, y, 0]) cylinder(h = size[2], r = r);
    }
}

module board_pod() {
    difference() {
        rounded_box([case_w, case_h, case_d], 7);

        // Back cavity accepts the PCB from behind, while the front face traps it.
        translate([(case_w - board_pocket_w) / 2, (case_h - board_pocket_h) / 2, back_thickness])
            rounded_box([board_pocket_w, board_pocket_h, board_depth + 0.25], 2);

        // Window through the watch face for the LCD glass.
        translate([
            (case_w - screen_w - screen_clearance * 2) / 2,
            (case_h - screen_h - screen_clearance * 2) / 2,
            back_thickness + board_depth - 0.1
        ])
            rounded_box([
                screen_w + screen_clearance * 2,
                screen_h + screen_clearance * 2,
                front_thickness + 0.3
            ], 2.2);

        // Bottom wire exit for power leads, with room for heat-shrink.
        translate([case_w / 2 - wire_hole_w / 2, -0.2, back_thickness + 1.2])
            cube([wire_hole_w, wall + 1.0, wire_hole_h]);
        translate([case_w / 2 - wire_hole_w / 2, -wire_channel_d, back_thickness + 1.2])
            cube([wire_hole_w, wire_channel_d + 0.4, wire_hole_h]);
    }

    // A raised rim makes the enclosure read more like a watch body.
    translate([face_rim / 2, face_rim / 2, case_d - 0.9])
        difference() {
            rounded_box([case_w - face_rim, case_h - face_rim, 0.9], 5.5);
            translate([
                (case_w - face_rim - screen_w - screen_clearance * 2 - 2.0) / 2,
                (case_h - face_rim - screen_h - screen_clearance * 2 - 2.0) / 2,
                -0.1
            ])
                rounded_box([
                    screen_w + screen_clearance * 2 + 2.0,
                    screen_h + screen_clearance * 2 + 2.0,
                    1.1
                ], 3);
        }

    for (x = [-hole_dx / 2, hole_dx / 2])
        for (y = [-hole_dy / 2, hole_dy / 2])
            translate([case_w / 2 + x, case_h / 2 + y, back_thickness])
                difference() {
                    cylinder(h = board_depth - 1.0, d = standoff_d);
                    translate([0, 0, -0.1])
                        cylinder(h = board_depth + 0.2, d = m2_clearance);
                }
}

module bar_clamp() {
    outer_d = bar_d + bar_clearance + clamp_thickness * 2;
    difference() {
        union() {
            rotate([90, 0, 0])
                cylinder(h = clamp_w, d = outer_d, center = true);
            translate([-outer_d / 2, -clamp_w / 2, -outer_d / 2])
                cube([outer_d, clamp_w, outer_d / 2]);
        }
        rotate([90, 0, 0])
            cylinder(h = clamp_w + 0.4, d = bar_d + bar_clearance, center = true);
        translate([-outer_d, -clamp_w / 2 - 0.2, -clamp_gap / 2])
            cube([outer_d * 2, clamp_w + 0.4, clamp_gap]);
        for (y = [-clamp_w / 4, clamp_w / 4]) {
            translate([0, y, -outer_d / 2 - 0.1])
                cylinder(h = outer_d + 0.2, d = clamp_bolt_d);
            translate([0, y, outer_d / 2 - clamp_nut_depth])
                cylinder(h = clamp_nut_depth + 0.2, d = clamp_nut_flat, $fn = 6);
        }
    }
}

module holder() {
    translate([-case_w / 2, 10, 0])
        rotate([tilt_deg, 0, 0])
            board_pod();

    translate([0, -10, -bar_d / 2 - 8])
        bar_clamp();

    hull() {
        translate([-case_w / 2 + 2, 10, 0])
            cube([case_w - 4, 5, back_thickness]);
        translate([-case_w / 2 + 2, -10, -bar_d / 2 - 3])
            cube([case_w - 4, 5, 4]);
    }
}

holder();
