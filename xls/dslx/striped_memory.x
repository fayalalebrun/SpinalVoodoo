import memory;

pub proc striped_word_memory<SIZE: u32> {
    command_r: chan<memory::MemoryCommand> in;
    response_s: chan<memory::MemoryResponse> out;
    c0s: chan<memory::MemoryCommand> out;
    c0r: chan<memory::MemoryResponse> in;
    c1s: chan<memory::MemoryCommand> out;
    c1r: chan<memory::MemoryResponse> in;
    c2s: chan<memory::MemoryCommand> out;
    c2r: chan<memory::MemoryResponse> in;
    c3s: chan<memory::MemoryCommand> out;
    c3r: chan<memory::MemoryResponse> in;

    config(command_r: chan<memory::MemoryCommand> in, response_s: chan<memory::MemoryResponse> out) {
        let (c0s, c0c) = chan<memory::MemoryCommand>("command0");
        let (r0c, c0r) = chan<memory::MemoryResponse>("response0");
        let (c1s, c1c) = chan<memory::MemoryCommand>("command1");
        let (r1c, c1r) = chan<memory::MemoryResponse>("response1");
        let (c2s, c2c) = chan<memory::MemoryCommand>("command2");
        let (r2c, c2r) = chan<memory::MemoryResponse>("response2");
        let (c3s, c3c) = chan<memory::MemoryCommand>("command3");
        let (r3c, c3r) = chan<memory::MemoryResponse>("response3");
        spawn memory::word_memory<{(SIZE + u32:3) / u32:4}>(c0c, r0c);
        spawn memory::word_memory<{(SIZE + u32:3) / u32:4}>(c1c, r1c);
        spawn memory::word_memory<{(SIZE + u32:3) / u32:4}>(c2c, r2c);
        spawn memory::word_memory<{(SIZE + u32:3) / u32:4}>(c3c, r3c);
        (command_r, response_s, c0s, c0r, c1s, c1r, c2s, c2r, c3s, c3r)
    }

    init { () }

    next(state: ()) {
        let default_command =
            memory::MemoryCommand { write: u1:0, address: u22:0, data: u32:0, mask: u4:0 };
        let (tok, command0) = recv(join(), command_r);
        let (tok, command1, valid1) = recv_non_blocking(tok, command_r, default_command);
        let (tok, command2, valid2) = recv_non_blocking(tok, command_r, default_command);
        let (tok, command3, valid3) = recv_non_blocking(tok, command_r, default_command);
        let commands = memory::MemoryCommand[4]:[command0, command1, command2, command3];
        let valid = u1[4]:[u1:1, valid1, valid2, valid3];
        let default_response = memory::MemoryResponse { data: u32:0, success: u1:0 };
        let tok = unroll_for! (i, tok): (u32, token) in u32:0..u32:4 {
            let command = commands[i];
            let bank = command.address[0+:u2];
            let local = memory::MemoryCommand { address: command.address >> u8:2, ..command };
            let active = valid[i] == u1:1;
            let tok = send_if(tok, c0s, active && bank == u2:0, local);
            let tok = send_if(tok, c1s, active && bank == u2:1, local);
            let tok = send_if(tok, c2s, active && bank == u2:2, local);
            let tok = send_if(tok, c3s, active && bank == u2:3, local);
            let (tok, r0) = recv_if(tok, c0r, active && bank == u2:0, default_response);
            let (tok, r1) = recv_if(tok, c1r, active && bank == u2:1, default_response);
            let (tok, r2) = recv_if(tok, c2r, active && bank == u2:2, default_response);
            let (tok, r3) = recv_if(tok, c3r, active && bank == u2:3, default_response);
            let response = match bank { u2:0 => r0, u2:1 => r1, u2:2 => r2, _ => r3 };
            send_if(tok, response_s, active, response)
        }(tok);
        state
    }
}
