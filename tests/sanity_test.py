#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from typing import Optional

# --- KONFIGURÁCIA ---
DEFAULT_COMPILER = "../ifj26"       # Cesta k vášmu skompilovanému prekladaču
DEFAULT_INTERPRETER = "./ic26int"   # Cesta k interpretu (voliteľné)
TIMEOUT_SEC = 2.0                   # Ochrana proti zacykleniu v lexeri/parseri
VALGRIND_TIMEOUT_SEC = 10.0

# ANSI farby pre terminál
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
RESET = "\033[0m"
PROLOG = "import ifj26.*\n"


@dataclass
class TestCase:
    name: str
    code: str
    expected_rc: int                     # Očakávaný návratový kód prekladača
    use_prolog: bool = True
    stdin_input: str = ""                # Vstup pre interpret (ak sa testuje aj beh)
    expected_out: Optional[str] = None   # Očakávaný stdout z interpretu (None = len preklad)


TESTS = [
    # --- 1. VALIDNÉ PROGRAMY (Očakávaný kód 0) ---
    TestCase(
        name="Valid: Minimálna funkcia main (telo s Unit literálom)",
        code="""
@main def main(): Unit =
    ()
""",
        expected_rc=0,
    ),
    TestCase(
        name="Valid: Aritmetika a implicitná konverzia Int -> Double",
        code="""
@main def main(): Unit =
    var a: Int = 5
    var b: Double = 2.5
    var c: Double = a + b
""",
        expected_rc=0,
    ),
    TestCase(
        name="Valid: Shadowing premennej vo vnorenom bloku",
        code="""
@main def main(): Unit =
    var x: Int = 10
    if x > 5 then
        var x: String = "vnorene"
    else
        ()
    var y: Int = x + 1
""",
        expected_rc=0,
    ),
    TestCase(
        name="Valid: Volanie funkcie s parametrami",
        code="""
def add(a: Int, b: Double): Double =
    return a + b

@main def main(): Unit =
    var res: Double = add(3, 4.5)
""",
        expected_rc=0,
    ),
    TestCase(
        name="Valid: Výpis hodnoty (E2E test pre interpret)",
        code="""
@main def main(): Unit =
    var a: Int = 40
    var b: Int = a + 2
    print(b)
""",
        expected_rc=0,
        expected_out="42",
    ),

    # --- 2. LEXIKÁLNE CHYBY (Kód 1) ---
    TestCase(
        name="Lex Error: Neukončený reťazec",
        code="""
@main def main(): Unit =
    var s: String = "neukonceny text
""",
        expected_rc=1,
    ),
    TestCase(
        name="Lex Error: Neplatný znak v kóde",
        code="""
@main def main(): Unit =
    var a: Int = 5 # 3
""",
        expected_rc=1,
    ),

    # --- 3. SYNTAKTICKÉ CHYBY (Kód 2) ---
    TestCase(
        name="Syntax Error: Chýbajúci povinný prológ",
        code="""
@main def main(): Unit =
    ()
""",
        expected_rc=2,
        use_prolog=False,
    ),
    TestCase(
        name="Syntax Error: Chýbajúca uzatváracia zátvorka",
        code="""
@main def main(): Unit =
    var a: Int = (5 + 3
""",
        expected_rc=2,
    ),
    TestCase(
        name="Syntax Error: Vnorená definícia funkcie",
        code="""
@main def main(): Unit =
    def nested(): Unit =
        ()
""",
        expected_rc=2,
    ),

    # --- 4. SÉMANTICKÉ CHYBY (Kódy podľa sekcie 6 zadania IFJ26) ---
    TestCase(
        name="Sem Error (3): Nedefinovaná premenná",
        code="""
@main def main(): Unit =
    var a: Int = neexistuje + 1
""",
        expected_rc=3,
    ),
    TestCase(
        name="Sem Error (3): Chýbajúca anotácia @main",
        code="""
def main(): Unit =
    ()
""",
        expected_rc=3,
    ),
    TestCase(
        name="Sem Error (4): Redefinícia premennej v rovnakom scope",
        code="""
@main def main(): Unit =
    var x: Int = 1
    var x: Int = 2
""",
        expected_rc=4,
    ),
    TestCase(
        name="Sem Error (4): Redefinícia funkcie",
        code="""
def foo(): Unit =
    ()

def foo(): Unit =
    ()

@main def main(): Unit =
    ()
""",
        expected_rc=4,
    ),
    TestCase(
        name="Sem Error (6): Typová nekompatibilita String + Int",
        code="""
@main def main(): Unit =
    var s: String = "ahoj" + 5
""",
        expected_rc=6,
    ),
    TestCase(
        name="Sem Error (6): Porovnávanie Boolean cez operátor <",
        code="""
@main def main(): Unit =
    if (1 < 2) < (3 < 4) then
        ()
    else
        ()
""",
        expected_rc=6,
    ),
    TestCase(
        name="Sem Error (6): Aritmetika s Boolean",
        code="""
@main def main(): Unit =
    var x: Int = (1 == 1) + 5
""",
        expected_rc=6,
    ),
]


def run_compiler(compiler_path: str, code: str, use_valgrind: bool):
    cmd = [compiler_path]
    timeout = TIMEOUT_SEC
    if use_valgrind:
        timeout = VALGRIND_TIMEOUT_SEC
        cmd = [
            "valgrind",
            "--leak-check=full",
            "--errors-for-leak-kinds=all",
            "--error-exitcode=111",
            "--quiet",
            compiler_path,
        ]

    try:
        proc = subprocess.run(
            cmd,
            input=code,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
            timeout=timeout,
        )
        return proc.returncode, proc.stdout, proc.stderr, False
    except subprocess.TimeoutExpired:
        return None, "", "TIMEOUT (pravdepodobné zacyklenie)", True


def run_interpreter(interp_path: str, ifjcode: str, stdin_input: str):
    with tempfile.NamedTemporaryFile("w", suffix=".ifjcode", delete=False) as tmp:
        tmp.write(ifjcode)
        tmp_path = tmp.name

    try:
        proc = subprocess.run(
            [interp_path, tmp_path],
            input=stdin_input,
            text=True,
            capture_output=True,
            timeout=TIMEOUT_SEC,
        )
        return proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "INTERPRETER TIMEOUT"
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)


def main():
    parser = argparse.ArgumentParser(description="E2E test runner pre IFJ26 prekladač")
    parser.add_argument("-c", "--compiler", default=DEFAULT_COMPILER, help="Cesta k binárke prekladača")
    parser.add_argument("-i", "--interpreter", default=None, help="Cesta k ic26int pre overenie behu")
    parser.add_argument("--valgrind", action="store_true", help="Spustiť testy pod Valgrindom")
    parser.add_argument("-v", "--verbose", action="store_true", help="Vypísať stderr/stdout aj pri chybe")
    args = parser.parse_args()

    if not os.path.isfile(args.compiler):
        print(f"{RED}Chyba: Binárka '{args.compiler}' neexistuje. Najprv spusti 'make'.{RESET}")
        sys.exit(1)

    if args.interpreter and not os.path.isfile(args.interpreter):
        print(f"{RED}Chyba: Interpret '{args.interpreter}' neexistuje.{RESET}")
        sys.exit(1)

    print(f"{CYAN}=== Spúšťam {len(TESTS)} E2E testov ({args.compiler}) ==={RESET}")
    if args.valgrind:
        print(f"{YELLOW}Režim: Valgrind memory check AKTÍVNY{RESET}")
    print()

    passed = 0
    failed = 0

    for idx, test in enumerate(TESTS, 1):
        body = test.code.lstrip("\n")
        full_code = (PROLOG + body) if test.use_prolog else body
        rc, stdout, stderr, timed_out = run_compiler(args.compiler, full_code, args.valgrind)

        # 1. Kontrola timeoutu
        if timed_out:
            print(f"{RED}[TIMEOUT]{RESET} #{idx:02d} {test.name}")
            failed += 1
            continue

        # 2. Kontrola pádu (SIGSEGV, SIGABRT - napr. double free)
        if rc < 0:
            print(f"{RED}[CRASH]  {RESET} #{idx:02d} {test.name} (Signal {-rc})")
            if args.verbose and stderr:
                print(f"          stderr: {stderr.strip()}")
            failed += 1
            continue

        # 3. Kontrola Valgrind chyby
        if args.valgrind and rc == 111:
            print(f"{RED}[MEMERR] {RESET} #{idx:02d} {test.name} (Valgrind našiel leak alebo invalid access)")
            if stderr:
                print(stderr)
            failed += 1
            continue

        # 4. Kontrola návratového kódu prekladača
        if rc != test.expected_rc:
            print(f"{RED}[FAIL]   {RESET} #{idx:02d} {test.name}")
            print(f"          Očakávaný RC: {test.expected_rc}, Skutočný RC: {rc}")
            if args.verbose and stderr:
                print(f"          stderr: {stderr.strip()}")
            failed += 1
            continue

        # 5. Voliteľné spustenie v interpretri (ak kód prešiel a test definuje expected_out)
        if args.interpreter and test.expected_rc == 0 and test.expected_out is not None:
            irc, iout, ierr = run_interpreter(args.interpreter, stdout, test.stdin_input)
            if irc != 0 or iout != test.expected_out:
                print(f"{RED}[RUNFAIL]{RESET} #{idx:02d} {test.name}")
                print(f"          Interpret RC: {irc}")
                print(f"          Očakávaný výstup: {repr(test.expected_out)}")
                print(f"          Skutočný výstup:  {repr(iout)}")
                if ierr:
                    print(f"          Interpret stderr: {ierr.strip()}")
                failed += 1
                continue

        print(f"{GREEN}[PASS]   {RESET} #{idx:02d} {test.name} (RC={rc})")
        passed += 1

    print()
    color = GREEN if failed == 0 else RED
    print(f"{color}Výsledok: {passed}/{len(TESTS)} prešlo, {failed} zlyhalo.{RESET}")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()