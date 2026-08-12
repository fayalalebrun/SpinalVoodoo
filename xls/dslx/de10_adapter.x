#![feature(channel_attributes)]

import memory;

pub struct MemoryTransfer { word_count: u7, command: memory::MemoryCommand }

pub struct MemoryTransferResponse { data: bits[2048], success: u1 }

fn empty_command() -> memory::MemoryCommand {
    memory::MemoryCommand { write: u1:0, address: u22:0, data: u32:0, mask: u4:0 }
}

fn empty_response() -> memory::MemoryResponse {
    memory::MemoryResponse { data: u32:0, success: u1:0 }
}

fn empty_transfer() -> MemoryTransfer {
    MemoryTransfer { word_count: u7:0, command: empty_command() }
}

fn empty_transfer_response() -> MemoryTransferResponse {
    MemoryTransferResponse { data: bits[2048]:0, success: u1:0 }
}

pub fn word_transfer(command: memory::MemoryCommand) -> MemoryTransfer {
    MemoryTransfer { word_count: u7:1, command }
}

pub fn block_read_transfer(block: memory::MemoryBlockRead) -> MemoryTransfer {
    MemoryTransfer {
        word_count: u7:64,
        command: memory::MemoryCommand {
            write: u1:0,
            address: block.address,
            data: u32:0,
            mask: u4:0xf,
        },
    }
}

pub fn word_response(response: MemoryTransferResponse) -> memory::MemoryResponse {
    memory::MemoryResponse { data: response.data as u32, success: response.success }
}

pub fn block_read_response(response: MemoryTransferResponse) -> memory::MemoryBlockReadResponse {
    memory::MemoryBlockReadResponse { data: response.data, success: response.success }
}

pub proc pair_to_planes {
    request_r: chan<memory::MemoryRequestPair> in;
    response_s: chan<memory::MemoryResponsePair> out;
    plane0_command_s: chan<memory::MemoryCommand> out;
    plane0_response_r: chan<memory::MemoryResponse> in;
    plane1_command_s: chan<memory::MemoryCommand> out;
    plane1_response_r: chan<memory::MemoryResponse> in;

    config(request_r: chan<memory::MemoryRequestPair> in,
           response_s: chan<memory::MemoryResponsePair> out,
           plane0_command_s: chan<memory::MemoryCommand> out,
           plane0_response_r: chan<memory::MemoryResponse> in,
           plane1_command_s: chan<memory::MemoryCommand> out,
           plane1_response_r: chan<memory::MemoryResponse> in) {
        (
            request_r, response_s, plane0_command_s, plane0_response_r, plane1_command_s,
            plane1_response_r,
        )
    }

    init { () }

    next(state: ()) {
        let (tok, request) = recv(join(), request_r);
        let valid0 = request.valid[0+:u1] == u1:1;
        let valid1 = request.valid[1+:u1] == u1:1;
        let tok = send_if(tok, plane0_command_s, valid0, request.commands[u32:0]);
        let tok = send_if(tok, plane1_command_s, valid1, request.commands[u32:1]);
        let (tok, response0) = recv_if(tok, plane0_response_r, valid0, empty_response());
        let (tok, response1) = recv_if(tok, plane1_response_r, valid1, empty_response());
        let tok = send(
            tok, response_s,
            memory::MemoryResponsePair {
                responses: memory::MemoryResponse[2]:[response0, response1],
            });
        state
    }
}

pub proc memory_arbiter3 {
    request0_r: chan<memory::MemoryCommand> in;
    response0_s: chan<memory::MemoryResponse> out;
    request1_r: chan<memory::MemoryCommand> in;
    response1_s: chan<memory::MemoryResponse> out;
    request2_r: chan<memory::MemoryCommand> in;
    response2_s: chan<memory::MemoryResponse> out;
    request_s: chan<memory::MemoryCommand> out;
    response_r: chan<memory::MemoryResponse> in;

    config(request0_r: chan<memory::MemoryCommand> in,
           response0_s: chan<memory::MemoryResponse> out,
           request1_r: chan<memory::MemoryCommand> in,
           response1_s: chan<memory::MemoryResponse> out,
           request2_r: chan<memory::MemoryCommand> in,
           response2_s: chan<memory::MemoryResponse> out,
           request_s: chan<memory::MemoryCommand> out,
           response_r: chan<memory::MemoryResponse> in) {
        (
            request0_r, response0_s, request1_r, response1_s, request2_r, response2_s, request_s,
            response_r,
        )
    }

    init { () }

    next(state: ()) {
        let (tok, request0, valid0) = recv_non_blocking(join(), request0_r, empty_command());
        let (tok, request1, valid1) = if valid0 {
            (tok, empty_command(), false)
        } else {
            recv_non_blocking(tok, request1_r, empty_command())
        };
        let (tok, request2, valid2) = if valid0 || valid1 {
            (tok, empty_command(), false)
        } else {
            recv_non_blocking(tok, request2_r, empty_command())
        };
        let valid = valid0 || valid1 || valid2;
        let selected = if valid0 { u2:0 } else if valid1 { u2:1 } else { u2:2 };
        let request = if valid0 { request0 } else if valid1 { request1 } else { request2 };
        let tok = send_if(tok, request_s, valid, request);
        let (tok, response) = recv_if(tok, response_r, valid, empty_response());
        let tok = send_if(tok, response0_s, valid && selected == u2:0, response);
        let tok = send_if(tok, response1_s, valid && selected == u2:1, response);
        let tok = send_if(tok, response2_s, valid && selected == u2:2, response);
        state
    }
}

pub proc word_transfer_adapter {
    command_r: chan<memory::MemoryCommand> in;
    response_s: chan<memory::MemoryResponse> out;
    transfer_s: chan<MemoryTransfer> out;
    transfer_response_r: chan<MemoryTransferResponse> in;

    config(command_r: chan<memory::MemoryCommand> in,
           response_s: chan<memory::MemoryResponse> out,
           transfer_s: chan<MemoryTransfer> out,
           transfer_response_r: chan<MemoryTransferResponse> in) {
        (command_r, response_s, transfer_s, transfer_response_r)
    }

    init { () }

    next(state: ()) {
        let (tok, command) = recv(join(), command_r);
        let tok = send(tok, transfer_s, word_transfer(command));
        let (tok, response) = recv(tok, transfer_response_r);
        let tok = send(tok, response_s, word_response(response));
        state
    }
}

pub proc block_read_transfer_adapter {
    command_r: chan<memory::MemoryBlockRead> in;
    response_s: chan<memory::MemoryBlockReadResponse> out;
    transfer_s: chan<MemoryTransfer> out;
    transfer_response_r: chan<MemoryTransferResponse> in;

    config(command_r: chan<memory::MemoryBlockRead> in,
           response_s: chan<memory::MemoryBlockReadResponse> out,
           transfer_s: chan<MemoryTransfer> out,
           transfer_response_r: chan<MemoryTransferResponse> in) {
        (command_r, response_s, transfer_s, transfer_response_r)
    }

    init { () }

    next(state: ()) {
        let (tok, command) = recv(join(), command_r);
        let tok = send(tok, transfer_s, block_read_transfer(command));
        let (tok, response) = recv(tok, transfer_response_r);
        let tok = send(tok, response_s, block_read_response(response));
        state
    }
}

pub proc transfer_arbiter2 {
    request0_r: chan<MemoryTransfer> in;
    response0_s: chan<MemoryTransferResponse> out;
    request1_r: chan<MemoryTransfer> in;
    response1_s: chan<MemoryTransferResponse> out;
    request_s: chan<MemoryTransfer> out;
    response_r: chan<MemoryTransferResponse> in;

    config(request0_r: chan<MemoryTransfer> in,
           response0_s: chan<MemoryTransferResponse> out,
           request1_r: chan<MemoryTransfer> in,
           response1_s: chan<MemoryTransferResponse> out,
           request_s: chan<MemoryTransfer> out,
           response_r: chan<MemoryTransferResponse> in) {
        (request0_r, response0_s, request1_r, response1_s, request_s, response_r)
    }

    init { () }

    next(state: ()) {
        let (tok, request0, valid0) = recv_non_blocking(join(), request0_r, empty_transfer());
        let (tok, request1, valid1) = if valid0 {
            (tok, empty_transfer(), false)
        } else {
            recv_non_blocking(tok, request1_r, empty_transfer())
        };
        let valid = valid0 || valid1;
        let request = if valid0 { request0 } else { request1 };
        let tok = send_if(tok, request_s, valid, request);
        let (tok, response) = recv_if(tok, response_r, valid, empty_transfer_response());
        let tok = send_if(tok, response0_s, valid0, response);
        let tok = send_if(tok, response1_s, valid1, response);
        state
    }
}

#[test]
fn transfer_shapes() {
    let command = memory::MemoryCommand {
        write: u1:1,
        address: u22:0x12345,
        data: u32:0x89abcdef,
        mask: u4:0x5,
    };
    assert_eq(word_transfer(command), MemoryTransfer { word_count: u7:1, command });
    assert_eq(
        block_read_transfer(memory::MemoryBlockRead { address: u22:0x345 }),
        MemoryTransfer {
            word_count: u7:64,
            command: memory::MemoryCommand {
                write: u1:0,
                address: u22:0x345,
                data: u32:0,
                mask: u4:0xf,
            },
        });
    let response = MemoryTransferResponse {
        data: (bits[2048]:0x12345678 << u12:32) | bits[2048]:0xdeadbeef,
        success: u1:1,
    };
    assert_eq(
        word_response(response),
        memory::MemoryResponse { data: u32:0xdeadbeef, success: u1:1 });
    assert_eq(block_read_response(response).data[32+:u32], u32:0x12345678);
}

#[test_proc]
proc memory_arbiter3_test {
    request1_s: chan<memory::MemoryCommand> out;
    response1_r: chan<memory::MemoryResponse> in;
    request_r: chan<memory::MemoryCommand> in;
    response_s: chan<memory::MemoryResponse> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (_, request0_r) = chan<memory::MemoryCommand>("request0");
        let (response0_s, _) = chan<memory::MemoryResponse>("response0");
        let (request1_s, request1_r) = chan<memory::MemoryCommand>("request1");
        let (response1_s, response1_r) = chan<memory::MemoryResponse>("response1");
        let (_, request2_r) = chan<memory::MemoryCommand>("request2");
        let (response2_s, _) = chan<memory::MemoryResponse>("response2");
        let (request_s, request_r) = chan<memory::MemoryCommand>("request");
        let (response_s, response_r) = chan<memory::MemoryResponse>("response");
        spawn memory_arbiter3(
            request0_r, response0_s, request1_r, response1_s, request2_r, response2_s, request_s,
            response_r);
        (request1_s, response1_r, request_r, response_s, terminator)
    }

    init { () }

    next(state: ()) {
        let command = memory::MemoryCommand {
            write: u1:0,
            address: u22:7,
            data: u32:0,
            mask: u4:0,
        };
        let tok = send(join(), request1_s, command);
        let (tok, forwarded) = recv(tok, request_r);
        assert_eq(forwarded, command);
        let expected = memory::MemoryResponse { data: u32:0xcafebabe, success: u1:1 };
        let tok = send(tok, response_s, expected);
        let (tok, response) = recv(tok, response1_r);
        assert_eq(response, expected);
        send(tok, terminator, true);
    }
}
