"""The installed arena must remain reachable, editable and self-contained."""
import collections
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / 'Source/Elements'


def load(name):
    return json.loads((DATA / 'Levels' / (name + '.json')).read_text(encoding='utf-8'))


class ArenaMapIntegration(unittest.TestCase):
    def test_combat_catalog_and_departures_use_the_new_arena(self):
        definition = json.loads((DATA / 'World/arena/arena-of-the-future.json').read_text(encoding='utf-8'))
        self.assertEqual(definition['map'], 'capital/arena-of-brave.json')
        arena = load('capital/arena-of-brave')
        zone = next(e for e in arena['entities'] if e['type']=='combatZone' and e['name']==definition['zone'])
        self.assertEqual((zone['width'],zone['height']), (20,14))
        grid = {(t['x'],t['y']):t['type'] for t in arena['tiles']}
        for side in ['allies','enemies']:
            entries = [e for e in arena['entities'] if e['type']=='arenaEntry' and e['side']==side]
            self.assertEqual(sorted(e['rank'] for e in entries), [1,2,3,4])
            for e in entries:
                self.assertNotEqual(grid.get((e['x'],e['y']), 'empty'), 'wall')
                self.assertTrue(zone['x'] <= e['x'] < zone['x']+zone['width'])
                self.assertTrue(zone['y'] <= e['y'] < zone['y']+zone['height'])
        self.assertIn('heraut-colisee', [e.get('dialogue') for e in arena['entities']])

    def test_every_scene_piece_is_installed(self):
        scene = DATA / 'Assets/Scene/arena-of-brave'
        catalog = json.loads((scene / 'editor-catalog.json').read_text())
        self.assertEqual(len(catalog['pieces']), 366)
        for name in catalog['pieces']:
            self.assertTrue((scene / (name + '.png')).is_file(), name)
        for name in ['arena-of-brave', 'arena-of-brave-camp-a', 'arena-of-brave-camp-b']:
            data = load('capital/' + name)
            self.assertEqual(data['version'], 4)
            for layer in data['layers']:
                for tile in layer['tiles']:
                    if 'piece' in tile:
                        self.assertTrue((scene / (tile['piece'] + '.png')).is_file(), tile)
            self.assertEqual(data['layers'][0]['scene'], 'arena-of-brave')

    def test_all_arena_doors_and_arrivals_reachable_without_crossing_walls(self):
        names = ['capital/' + n for n in ['arena-of-brave', 'arena-of-brave-camp-a',
                 'arena-of-brave-camp-b', 'arenarea']]
        maps = {n: load(n) for n in names}
        grids = {n: {(t['x'], t['y']): t['type'] for t in d['tiles']} for n, d in maps.items()}
        portals = {(n, e['x'], e['y']): e for n, d in maps.items() for e in d['entities'] if e['type']=='portal'}
        start = ('capital/arena-of-brave', 44, 44)
        seen = {start}
        queue = collections.deque([start])
        while queue:
            n, x, y = queue.popleft()
            neighbors = [(n, x-1, y), (n, x+1, y), (n, x, y-1), (n, x, y+1)]
            door = portals.get((n,x,y))
            if door and door['targetMap'] in maps:
                target = door['targetMap']
                arrival = next(e for e in maps[target]['entities'] if e['type']=='spawnPoint' and e['name']==door['arrival'])
                neighbors.append((target, arrival['x'], arrival['y']))
            for cell in neighbors:
                target, cx, cy = cell
                d = maps[target]
                if cell not in seen and 0 <= cx < d['width'] and 0 <= cy < d['height'] and grids[target].get((cx,cy),'empty') not in ('wall','solid','water'):
                    seen.add(cell)
                    queue.append(cell)
        for name, data in maps.items():
            for e in data['entities']:
                if e['type'] in ('portal','spawnPoint'):
                    self.assertTrue((name,e['x'],e['y']) in seen, e)
        for name,x,y in seen:
            self.assertNotEqual(grids[name].get((x,y)), 'wall')


if __name__ == '__main__':
    unittest.main()
