#!/usr/bin/env python3
"""
Scrape full year 2026 tide data for Belle-Ile (Le Palais) from maree.info
Uses the internal /do/load-maree-jours.php API (no browser needed).
~53 weekly requests instead of 365 per-day page loads.
"""

import datetime as dt
import csv
import re
import requests
from bs4 import BeautifulSoup

PORT = 101        # maree.info port ID for Le Palais
YEAR = 2026
OUTPUT_CSV = 'mareeinfo_le_palais_2026.csv'


def get_session():
    s = requests.Session()
    s.headers.update({
        'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64)',
        'Referer': 'https://maree.info/',
    })
    s.get(f'https://maree.info/{PORT}', timeout=10)
    return s


def fetch_week(session, week_start: dt.date):
    r = session.get(
        'https://maree.info/do/load-maree-jours.php',
        params={'p': PORT, 'd': week_start.strftime('%Y%m%d'), 'j': 0},
        timeout=15
    )
    r.raise_for_status()
    return r.text


def parse_week(js_text: str, year_filter: int):
    """
    Parse the JS response. The table is in e.innerHTML="<table>...</table>".
    Each row: <th>=date, <td>[0]=times, <td>[1]=heights, <td>[2]=coeffs.
    High tides are in <b> bold; coefficients align with bold entries.
    """
    m = re.search(r'Marees\.Dates\s*=\s*(\[[^\]]+\])', js_text)
    if not m:
        return []

    dates_raw = re.findall(r'\d{8}', m.group(1))
    dates = [dt.date(int(d[:4]), int(d[4:6]), int(d[6:8])) for d in dates_raw]

    m2 = re.search(r'e\.innerHTML\s*=\s*"(.*?)"\s*;?\s*\n', js_text, re.DOTALL)
    if not m2:
        return []

    inner = (m2.group(1)
               .replace('\\"', '"')
               .replace("\\'", "'")
               .replace('\\n', '\n')
               .replace('\\/', '/'))

    soup = BeautifulSoup(inner, 'html.parser')
    rows = soup.find_all('tr', class_=re.compile(r'\bMJ\b'))

    tides = []
    for day_idx, row in enumerate(rows):
        if day_idx >= len(dates):
            break

        date = dates[day_idx]
        if date.year != year_filter:
            continue

        cells = row.find_all('td')
        if len(cells) < 3:
            continue

        # Split each cell by <br/> to get per-tide values
        time_parts = str(cells[0]).split('<br/>')
        height_parts = str(cells[1]).split('<br/>')
        coeff_parts = str(cells[2]).split('<br/>')

        for i, (tp, hp, cp) in enumerate(zip(time_parts, height_parts, coeff_parts)):
            # Extract time text
            time_text = BeautifulSoup(tp, 'html.parser').get_text().strip()
            if not time_text or time_text == '--h--':
                continue

            # Extract height value
            height_text = BeautifulSoup(hp, 'html.parser').get_text().strip()
            try:
                time_obj = dt.datetime.strptime(time_text, '%Hh%M').time()
                combined = dt.datetime.combine(date, time_obj)
                level = float(height_text.replace(',', '.').replace('m', ''))
            except ValueError:
                continue

            # High tides are bold (<b>); coefficient is also bold in coeff cell
            is_high = '<b>' in tp
            coeff = None
            coeff_match = re.search(r'<b>(\d+)</b>', cp)
            if coeff_match:
                coeff = int(coeff_match.group(1))

            tides.append((combined, level, coeff, is_high))

    return tides


def main():
    start = dt.date(YEAR, 1, 1)
    end = dt.date(YEAR, 12, 31)

    week_starts = []
    d = start
    while d <= end:
        week_starts.append(d)
        d += dt.timedelta(days=7)

    print(f'Fetching {len(week_starts)} weeks...')

    session = get_session()
    all_tides = []
    seen = set()

    for i, week_start in enumerate(week_starts):
        print(f'[{i+1:2d}/{len(week_starts)}] Week of {week_start}', end='', flush=True)
        try:
            js_text = fetch_week(session, week_start)
            week_tides = parse_week(js_text, YEAR)
            new_count = 0
            for entry in week_tides:
                key = entry[0]
                if key not in seen:
                    seen.add(key)
                    all_tides.append(entry)
                    new_count += 1
            print(f'  -> {new_count} new tides')
        except Exception as e:
            print(f'  ERROR: {e}')

    all_tides.sort(key=lambda x: x[0])

    with open(OUTPUT_CSV, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['datetime', 'height_m', 'is_high_tide', 'coefficient'])
        for dt_val, level, coeff, is_high in all_tides:
            writer.writerow([
                dt_val.strftime('%Y-%m-%d %H:%M'),
                level,
                'H' if is_high else 'L',
                coeff if coeff is not None else ''
            ])

    print(f'\nDone! {len(all_tides)} tide events written to {OUTPUT_CSV}')


if __name__ == '__main__':
    main()
