#!/usr/bin/env python3

import os
import curses
import time
import argparse
import random

class TimeCardManager:
    BASE_PATH = "/sys/class/timecard"

    def __init__(self, demo_mode=False):
        self.demo_mode = demo_mode
        self.cards = []
        self.mock_data = {}
        if self.demo_mode:
            self._init_mock_data()
        else:
            self.refresh_cards()

    def _init_mock_data(self):
        self.cards = ["ocp0", "ocp1"]
        for card in self.cards:
            self.mock_data[card] = {
                "serialnum": f"TC-{random.randint(1000, 9999)}",
                "gnss_sync": "1",
                "clock_source": "GNSS",
                "available_clock_sources": "GNSS ATOMIC PTP EXTERNAL",
                "clock_status_drift": "0.123",
                "clock_status_offset": "0.005",
                "sma1": "PPS_OUT",
                "sma2": "10MHz_OUT",
                "sma3": "PPS_IN",
                "sma4": "NONE",
                "available_sma_inputs": "PPS_IN 10MHz_IN NONE",
                "available_sma_outputs": "PPS_OUT 10MHz_OUT NONE",
                "utc_tai_offset": "37",
                "external_pps_cable_delay": "100",
                "internal_pps_cable_delay": "50",
                "irig_b_mode": "B002",
                "tod_protocol": "NMEA",
                "available_tod_protocols": "NMEA UBX TSIP",
                "tod_baud_rate": "115200",
                "available_tod_baud_rates": "9600 19200 38400 57600 115200",
                "tod_correction": "0"
            }

    def refresh_cards(self):
        if self.demo_mode:
            return
        if not os.path.exists(self.BASE_PATH):
            self.cards = []
            return
        self.cards = sorted([d for d in os.listdir(self.BASE_PATH) if d.startswith("ocp")])

    def get_attr(self, card, attr):
        if self.demo_mode:
            # Simulate slight drift/offset changes
            if attr == "clock_status_drift":
                val = float(self.mock_data[card][attr]) + (random.random() - 0.5) * 0.01
                self.mock_data[card][attr] = f"{val:.4f}"
            elif attr == "clock_status_offset":
                val = float(self.mock_data[card][attr]) + (random.random() - 0.5) * 0.001
                self.mock_data[card][attr] = f"{val:.4f}"
            return self.mock_data[card].get(attr, "N/A")
        
        path = os.path.join(self.BASE_PATH, card, attr)
        try:
            with open(path, "r") as f:
                return f.read().strip()
        except Exception:
            return "N/A"

    def set_attr(self, card, attr, value):
        if self.demo_mode:
            self.mock_data[card][attr] = str(value)
            return True
        
        path = os.path.join(self.BASE_PATH, card, attr)
        try:
            with open(path, "w") as f:
                f.write(str(value))
            return True
        except Exception as e:
            return False

class TimeCardUI:
    def __init__(self, stdscr, manager):
        self.stdscr = stdscr
        self.manager = manager
        self.current_card_idx = 0
        self.running = True
        self.menu_idx = 0
        self.sub_menu_idx = 0
        self.mode = "DASHBOARD" # DASHBOARD, CONFIG_SMA, CONFIG_TIMING, CONFIG_TOD
        
        # UI colors
        curses.start_color()
        curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK) # Header
        curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK) # Values
        curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK) # Selection
        curses.init_pair(4, curses.COLOR_RED, curses.COLOR_BLACK) # Errors
        curses.init_pair(5, curses.COLOR_WHITE, curses.COLOR_BLUE) # Highlight

    def draw_header(self):
        h, w = self.stdscr.getmaxyx()
        title = " TimeCard Ncurses Configurator "
        self.stdscr.attron(curses.color_pair(5) | curses.A_BOLD)
        self.stdscr.addstr(0, (w - len(title)) // 2, title)
        self.stdscr.attroff(curses.color_pair(5) | curses.A_BOLD)
        
        status_line = f" Card: {self.get_current_card()} | Mode: {self.mode} | Press 'q' to quit, 'TAB' to switch card "
        self.stdscr.addstr(1, 0, status_line[:w-1])
        self.stdscr.addstr(2, 0, "=" * w)

    def get_current_card(self):
        if not self.manager.cards:
            return "No Cards Found"
        return self.manager.cards[self.current_card_idx]

    def draw_dashboard(self):
        self.stdscr.attron(curses.A_BOLD)
        self.stdscr.addstr(4, 2, "--- Dashboard ---")
        self.stdscr.attroff(curses.A_BOLD)

        card = self.get_current_card()
        if card == "No Cards Found":
            self.stdscr.addstr(6, 5, "No TimeCard devices found in /sys/class/timecard/", curses.color_pair(4))
            return

        attrs = [
            ("Serial Number", "serialnum"),
            ("GNSS Sync Status", "gnss_sync"),
            ("Clock Source", "clock_source"),
            ("Clock Drift", "clock_status_drift"),
            ("Clock Offset", "clock_status_offset"),
            ("UTC/TAI Offset", "utc_tai_offset")
        ]

        for i, (label, attr) in enumerate(attrs):
            val = self.manager.get_attr(card, attr)
            self.stdscr.addstr(6 + i, 4, f"{label:20}: ")
            self.stdscr.addstr(val, curses.color_pair(2) | curses.A_BOLD)

        self.stdscr.addstr(14, 2, "Menu (Use Arrow Keys & Enter):")
        menu_items = ["Dashboard", "SMA Configuration", "Timing Configuration", "ToD Configuration"]
        for i, item in enumerate(menu_items):
            style = curses.A_REVERSE if (self.menu_idx == i and self.mode == "DASHBOARD") else curses.A_NORMAL
            self.stdscr.addstr(16 + i, 4, f"{i+1}. {item}", style)

    def draw_sma_config(self):
        card = self.get_current_card()
        self.stdscr.attron(curses.A_BOLD)
        self.stdscr.addstr(4, 2, f"--- SMA Configuration for {card} ---")
        self.stdscr.attroff(curses.A_BOLD)
        
        smas = [("SMA1", "sma1"), ("SMA2", "sma2"), ("SMA3", "sma3"), ("SMA4", "sma4")]
        for i, (label, attr) in enumerate(smas):
            val = self.manager.get_attr(card, attr)
            style = curses.A_REVERSE if self.sub_menu_idx == i else curses.A_NORMAL
            self.stdscr.addstr(6 + i, 4, f"{label:5}: {val}", style)

        self.stdscr.addstr(12, 2, "Selection Menu:")
        self.stdscr.addstr(13, 4, "- Arrow UP/DOWN to select SMA")
        self.stdscr.addstr(14, 4, "- 'Enter' to cycle values")
        self.stdscr.addstr(15, 4, "- 'Backspace' to return")

    def draw_timing_config(self):
        card = self.get_current_card()
        self.stdscr.attron(curses.A_BOLD)
        self.stdscr.addstr(4, 2, f"--- Timing Configuration for {card} ---")
        self.stdscr.attroff(curses.A_BOLD)

        attrs = [
            ("External PPS Delay", "external_pps_cable_delay"),
            ("Internal PPS Delay", "internal_pps_cable_delay"),
            ("UTC/TAI Offset", "utc_tai_offset"),
            ("IRIG-B Mode", "irig_b_mode")
        ]

        for i, (label, attr) in enumerate(attrs):
            val = self.manager.get_attr(card, attr)
            style = curses.A_REVERSE if self.sub_menu_idx == i else curses.A_NORMAL
            self.stdscr.addstr(6 + i, 4, f"{label:20}: {val}", style)

        self.stdscr.addstr(12, 2, "Selection Menu:")
        self.stdscr.addstr(13, 4, "- 'Enter' to edit value")
        self.stdscr.addstr(14, 4, "- 'Backspace' to return")

    def draw_tod_config(self):
        card = self.get_current_card()
        self.stdscr.attron(curses.A_BOLD)
        self.stdscr.addstr(4, 2, f"--- ToD Configuration for {card} ---")
        self.stdscr.attroff(curses.A_BOLD)

        attrs = [
            ("ToD Protocol", "tod_protocol"),
            ("ToD Baud Rate", "tod_baud_rate"),
            ("ToD Correction", "tod_correction")
        ]

        for i, (label, attr) in enumerate(attrs):
            val = self.manager.get_attr(card, attr)
            style = curses.A_REVERSE if self.sub_menu_idx == i else curses.A_NORMAL
            self.stdscr.addstr(6 + i, 4, f"{label:20}: {val}", style)

        self.stdscr.addstr(12, 2, "Selection Menu:")
        self.stdscr.addstr(13, 4, "- 'Enter' to cycle/edit values")
        self.stdscr.addstr(14, 4, "- 'Backspace' to return")

    def edit_value(self, card, attr, label):
        curses.echo()
        curses.curs_set(1)
        self.stdscr.addstr(18, 2, f"Enter new value for {label}: ")
        self.stdscr.refresh()
        new_val = self.stdscr.getstr(18, 30 + len(label)).decode('utf-8')
        curses.noecho()
        curses.curs_set(0)
        if new_val:
            self.manager.set_attr(card, attr, new_val)

    def cycle_attr(self, card, attr, available_attr):
        current = self.manager.get_attr(card, attr)
        available = self.manager.get_attr(card, available_attr).split()
        if current in available:
            idx = (available.index(current) + 1) % len(available)
            self.manager.set_attr(card, attr, available[idx])
        elif available:
            self.manager.set_attr(card, attr, available[0])

    def run(self):
        self.stdscr.nodelay(True)
        curses.curs_set(0)
        while self.running:
            self.stdscr.erase()
            self.draw_header()
            
            if self.mode == "DASHBOARD":
                self.draw_dashboard()
            elif self.mode == "CONFIG_SMA":
                self.draw_sma_config()
            elif self.mode == "CONFIG_TIMING":
                self.draw_timing_config()
            elif self.mode == "CONFIG_TOD":
                self.draw_tod_config()

            self.stdscr.refresh()
            
            try:
                ch = self.stdscr.getch()
                if ch == ord('q'):
                    self.running = False
                elif ch == ord('\t'):
                    if self.manager.cards:
                        self.current_card_idx = (self.current_card_idx + 1) % len(self.manager.cards)
                elif ch == curses.KEY_DOWN:
                    if self.mode == "DASHBOARD":
                        self.menu_idx = (self.menu_idx + 1) % 4
                    else:
                        limit = 4 if self.mode == "CONFIG_SMA" else (4 if self.mode == "CONFIG_TIMING" else 3)
                        self.sub_menu_idx = (self.sub_menu_idx + 1) % limit
                elif ch == curses.KEY_UP:
                    if self.mode == "DASHBOARD":
                        self.menu_idx = (self.menu_idx - 1) % 4
                    else:
                        limit = 4 if self.mode == "CONFIG_SMA" else (4 if self.mode == "CONFIG_TIMING" else 3)
                        self.sub_menu_idx = (self.sub_menu_idx - 1) % limit
                elif ch == 10: # Enter
                    card = self.get_current_card()
                    if card == "No Cards Found":
                        continue
                    if self.mode == "DASHBOARD":
                        if self.menu_idx == 0: self.mode = "DASHBOARD"
                        elif self.menu_idx == 1: self.mode = "CONFIG_SMA"; self.sub_menu_idx = 0
                        elif self.menu_idx == 2: self.mode = "CONFIG_TIMING"; self.sub_menu_idx = 0
                        elif self.menu_idx == 3: self.mode = "CONFIG_TOD"; self.sub_menu_idx = 0
                    elif self.mode == "CONFIG_SMA":
                        smas = ["sma1", "sma2", "sma3", "sma4"]
                        attr = smas[self.sub_menu_idx]
                        available = self.manager.get_attr(card, "available_sma_inputs").split() + \
                                    self.manager.get_attr(card, "available_sma_outputs").split()
                        current = self.manager.get_attr(card, attr)
                        if current in available:
                            idx = (available.index(current) + 1) % len(available)
                            self.manager.set_attr(card, attr, available[idx])
                        elif available:
                            self.manager.set_attr(card, attr, available[0])
                    elif self.mode == "CONFIG_TIMING":
                        attrs = [
                            ("external_pps_cable_delay", "External PPS Delay"),
                            ("internal_pps_cable_delay", "Internal PPS Delay"),
                            ("utc_tai_offset", "UTC/TAI Offset"),
                            ("irig_b_mode", "IRIG-B Mode")
                        ]
                        attr, label = attrs[self.sub_menu_idx]
                        self.edit_value(card, attr, label)
                    elif self.mode == "CONFIG_TOD":
                        attrs = [
                            ("tod_protocol", "ToD Protocol"),
                            ("tod_baud_rate", "ToD Baud Rate"),
                            ("tod_correction", "ToD Correction")
                        ]
                        if self.sub_menu_idx == 0:
                            self.cycle_attr(card, "tod_protocol", "available_tod_protocols")
                        elif self.sub_menu_idx == 1:
                            self.cycle_attr(card, "tod_baud_rate", "available_tod_baud_rates")
                        elif self.sub_menu_idx == 2:
                            attr, label = attrs[self.sub_menu_idx]
                            self.edit_value(card, attr, label)
                
                elif ch == curses.KEY_BACKSPACE or ch == 127: # Backspace
                    self.mode = "DASHBOARD"
                
                time.sleep(0.05)
            except KeyboardInterrupt:
                break

def main(stdscr, demo_mode):
    manager = TimeCardManager(demo_mode=demo_mode)
    ui = TimeCardUI(stdscr, manager)
    ui.run()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TimeCard Ncurses Configurator")
    parser.add_argument("--demo", action="store_true", help="Run in demo mode with mock data")
    args = parser.parse_args()
    
    curses.wrapper(main, args.demo)
