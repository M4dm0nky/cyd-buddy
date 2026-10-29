#pragma once
bool camera_start();    // true = Sensor läuft (QVGA, RGB565)
void camera_stop();     // XCLK aus, Puffer frei (PWDN ist beim XIAO nicht verdrahtet)
bool camera_running();
