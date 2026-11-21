mod scene;

use scene::{Editor, Rect};
use std::io::{self, BufRead, IsTerminal, Write};

fn help() {
    print!(
        "COMMANDS (names are single words; coordinates are integers)\n\
         \x20 add PARENT NAME X Y WIDTH HEIGHT\n\
         \x20 move NAME DX DY       worldpos NAME\n\
         \x20 intersects A B        swap A B\n\
         \x20 erase NAME            tree\n\
         \x20 select NAME           info\n\
         \x20 delete-selected       cut\n\
         \x20 paste PARENT          clear-clipboard\n\
         \x20 unselect              help / quit\n"
    );
}

fn integer(s: &str) -> Option<i32> {
    s.parse::<i32>().ok()
}

fn run_command(editor: &mut Editor, tokens: &[&str]) -> bool {
    match tokens {
        ["quit"] | ["exit"] => return false,
        ["help"] => help(),
        ["tree"] => editor.print_tree(),
        ["add", parent, name, x, y, w, h] => {
            if let (Some(x), Some(y), Some(w), Some(h)) =
                (integer(x), integer(y), integer(w), integer(h))
            {
                println!("{}", if editor.add(parent, name, Rect { x, y, width: w, height: h }) { "OK" } else { "ERR" });
            } else {
                println!("ERR integer argument");
            }
        }
        ["move", name, dx, dy] => {
            if let (Some(dx), Some(dy)) = (integer(dx), integer(dy)) {
                println!("{}", if editor.move_node(name, dx, dy) { "OK" } else { "ERR" });
            } else { println!("ERR integer argument"); }
        }
        ["worldpos", name] => match editor.world_position(name) {
            Some(p) => println!("WORLD {} {}", p.x, p.y),
            None => println!("ERR"),
        },
        ["intersects", a, b] => match editor.intersects(a, b) {
            Some(true) => println!("YES"),
            Some(false) => println!("NO"),
            None => println!("ERR"),
        },
        ["swap", a, b] => println!("{}", if editor.swap_positions(a, b) { "OK" } else { "ERR" }),
        ["erase", name] => println!("{}", if editor.erase(name) { "OK" } else { "ERR" }),
        ["select", name] => println!("{}", if editor.select(name) { "OK" } else { "ERR" }),
        ["info"] => println!("{}", editor.info()),
        ["delete-selected"] => println!("{}", if editor.delete_selected() { "OK" } else { "ERR" }),
        ["cut"] => println!("{}", if editor.cut() { "OK" } else { "ERR" }),
        ["paste", parent] => println!("{}", if editor.paste(parent) { "OK" } else { "ERR" }),
        ["unselect"] => { editor.unselect(); println!("OK"); }
        ["clear-clipboard"] => { editor.clear_clipboard(); println!("OK"); }
        _ => println!("ERR usage (type help)"),
    }
    true
}

fn main() {
    let mut editor = Editor::new();
    let stdin = io::stdin();
    let prompt = stdin.is_terminal();
    let mut input = stdin.lock();
    if prompt { help(); }
    loop {
        if prompt {
            print!("scene> ");
            let _ = io::stdout().flush();
        }
        let mut line = String::new();
        match input.read_line(&mut line) {
            Ok(0) | Err(_) => break,
            Ok(_) => {}
        }
        let tokens: Vec<&str> = line.split_whitespace().take_while(|t| !t.starts_with('#')).collect();
        if tokens.is_empty() { continue; }
        if !run_command(&mut editor, &tokens) { break; }
    }
}
