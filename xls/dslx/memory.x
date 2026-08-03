pub struct MemoryCommand { write: u1, address: u22, data: u32, mask: u4 }

pub struct MemoryResponse { data: u32, success: u1 }

pub struct MemoryBlockWrite { address: u22, data: u32[32], masks: u4[32] }

pub struct MemoryBlockResponse { success: u1 }

fn byte_mask(mask: u4) -> u32 {
    (if mask[0+:u1] == u1:1 { u32:0x000000ff } else { u32:0 }) |
    (if mask[1+:u1] == u1:1 { u32:0x0000ff00 } else { u32:0 }) |
    (if mask[2+:u1] == u1:1 { u32:0x00ff0000 } else { u32:0 }) |
    (if mask[3+:u1] == u1:1 { u32:0xff000000 } else { u32:0 })
}

fn masked_write(current: u32, data: u32, mask: u4) -> u32 {
    let expanded = byte_mask(mask);
    (current & !expanded) | (data & expanded)
}

pub proc word_memory<SIZE: u32> {
    command_r: chan<MemoryCommand> in;
    response_s: chan<MemoryResponse> out;
    type Memory = u32[SIZE];

    config(command_r: chan<MemoryCommand> in, response_s: chan<MemoryResponse> out) {
        (command_r, response_s)
    }

    init { Memory:[u32:0, ...] }

    next(memory: Memory) {
        let (tok, command) = recv(join(), command_r);
        let current = memory[command.address];
        let data = if command.write == u1:1 {
            masked_write(current, command.data, command.mask)
        } else {
            current
        };
        let memory =
            if command.write == u1:1 { update(memory, command.address, data) } else { memory };
        let tok = send(tok, response_s, MemoryResponse { data, success: u1:1 });
        memory
    }
}

#[test_proc]
proc word_memory_test {
    command_s: chan<MemoryCommand> out;
    response_r: chan<MemoryResponse> in;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (command_s, command_r) = chan<MemoryCommand>("command");
        let (response_s, response_r) = chan<MemoryResponse>("response");
        spawn word_memory<u32:16>(command_r, response_s);
        (command_s, response_r, terminator)
    }

    init { () }

    next(state: ()) {
        let write =
            MemoryCommand { write: u1:1, address: u22:3, data: u32:0x12345678, mask: u4:0x5 };
        let tok = send(join(), command_s, write);
        let (tok, response) = recv(tok, response_r);
        assert_eq(response, MemoryResponse { data: u32:0x00340078, success: u1:1 });
        let read = MemoryCommand { write: u1:0, address: u22:3, data: u32:0, mask: u4:0 };
        let tok = send(tok, command_s, read);
        let (tok, response) = recv(tok, response_r);
        assert_eq(response, MemoryResponse { data: u32:0x00340078, success: u1:1 });
        send(tok, terminator, true);
    }
}
