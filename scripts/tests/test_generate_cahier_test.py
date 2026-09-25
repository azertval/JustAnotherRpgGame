# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le générateur du cahier de test : ce qu'il relève dans un test, et ce qu'il en écrit.

Le cahier est engendré ; personne ne le relit ligne à ligne. Ces tests gardent les deux
inférences qui pourraient se tromper en silence — quelles exigences un cas cite, et quelles
exigences la matrice compte comme en vigueur.
"""
import textwrap

import generate_cahier_test as gen

TEST_SOURCE = textwrap.dedent('''\
    /**
     * @brief Un jet se restitue (EX-REG-003).
     * \\castest{<b>Le journal dit le de et les modificateurs.</b><br/>
     * \\tcat Unitaire · Combat<br/>
     * \\tcrit Critique<br/>
     * \\tetapes 1. Le d20 est force a 12.<br/>
     * \\tattendu Une ligne de journal.
     * }
     */
    TEST(AttackTest, LeJetSeRestitue) {
        EXPECT_EQ(journal.size(), 1U);  // EX-CBT-030
    }

    /**
     * @brief Un cas qui ne cite rien.
     * \\castest{<b>Rien.</b><br/>
     * \\tcat Unitaire · Combat<br/>
     * \\tcrit Mineur<br/>
     * \\tetapes 1. Rien.<br/>
     * \\tattendu Rien.
     * }
     */
    TEST(AttackTest, RienDuTout) {
        EXPECT_TRUE(true);
    }
    ''')


def _cases(tmp_path):
    root = tmp_path / 'Unit' / 'Core' / 'Combat'
    root.mkdir(parents=True)
    (root / 'test_attack.cpp').write_text(TEST_SOURCE, encoding='utf-8')
    return gen.collect_cases(str(tmp_path))


def test_un_cas_cite_les_exigences_de_son_commentaire_et_de_son_corps(tmp_path):
    cases = _cases(tmp_path)
    assert [case['name'] for case in cases] == ['LeJetSeRestitue', 'RienDuTout']
    assert cases[0]['exigences'] == ['EX-CBT-030', 'EX-REG-003']
    # Le commentaire du premier test ne déteint pas sur le second.
    assert cases[1]['exigences'] == []


def test_la_fiche_ecrit_ses_exigences_en_code(tmp_path):
    fiche = '\n'.join(gen.render_case(_cases(tmp_path)[0]))
    assert 'Exigences : `EX-CBT-030`, `EX-REG-003`' in fiche


def test_les_exigences_retirees_ne_comptent_pas_dans_la_matrice(tmp_path):
    spec = tmp_path / 'spec'
    spec.mkdir()
    (spec / 'combat.md').write_text(textwrap.dedent('''\
        # Combat tactique

        - **EX-CBT-030** — Une attaque est un jet.
        - **EX-CBT-099** *(retirée au `LOT-88`)* — plus rien.
        '''), encoding='utf-8')
    (spec / 'README.md').write_text('# Spécifications\n\n- **EX-CBT-030** — doublon ignoré.\n',
                                     encoding='utf-8')
    exigences = gen.collect_exigences(str(spec))
    assert exigences == {'EX-CBT-030': ('combat.md', 'Combat tactique', False),
                         'EX-CBT-099': ('combat.md', 'Combat tactique', True)}

    cases = _cases(tmp_path)
    domains = {'core-combat': ('Core · Combat', 'Unit', cases)}
    page = gen.render_coverage(domains, exigences)
    assert '**1 exigences en vigueur sur 1**' in page
    assert 'EX-CBT-099' not in page
    assert '[`AttackTest.LeJetSeRestitue`](core-combat.md#attacktestlejetserestitue)' in page
    # EX-REG-003 est citée sans être déclarée : la matrice le dit.
    assert 'déclarées nulle part' in page and '`EX-REG-003`' in page
