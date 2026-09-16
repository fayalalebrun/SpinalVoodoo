#![feature(channel_attributes)]

import memory;

pub struct MemoryTransfer { word_count: u7, command: memory::MemoryCommand }

pub struct MemoryTransferResponse { data: u32, success: u1 }

struct TransferRoute { client: u1, response_count: u7 }

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
    MemoryTransferResponse { data: u32:0, success: u1:0 }
}

pub fn word_transfer(command: memory::MemoryCommand) -> MemoryTransfer {
    MemoryTransfer { word_count: u7:1, command }
}

pub fn block_read_transfer<WORDS: u7 = {u7:64}>(block: memory::MemoryBlockRead) -> MemoryTransfer {
    MemoryTransfer {
        word_count: WORDS,
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

pub fn block_read_response(response: MemoryTransferResponse)
    -> memory::MemoryBlockReadWordResponse {
    memory::MemoryBlockReadWordResponse { data: response.data, success: response.success }
}

#[test]
fn sector_transfer_uses_sixteen_words_without_changing_full_line_default() {
    let command = memory::MemoryBlockRead { address: u22:0x350 };
    let full = block_read_transfer(command);
    let sector = block_read_transfer<u7:16>(command);
    assert_eq(full.word_count, u7:64);
    assert_eq(sector.word_count, u7:16);
    assert_eq(sector.command, full.command);
    assert_eq(sector.command.address, command.address);
    assert_eq(block_read_response(MemoryTransferResponse { data: u32:123, success: u1:0 }),
        memory::MemoryBlockReadWordResponse { data: u32:123, success: u1:0 });
}

proc pair_to_planes_requests {
    request_r: chan<memory::MemoryRequestPair> in;
    plane0_command_s: chan<memory::MemoryCommand> out;
    plane1_command_s: chan<memory::MemoryCommand> out;
    route_s: chan<u2> out;

    config(request_r: chan<memory::MemoryRequestPair> in,
           plane0_command_s: chan<memory::MemoryCommand> out,
           plane1_command_s: chan<memory::MemoryCommand> out, route_s: chan<u2> out) {
        (request_r, plane0_command_s, plane1_command_s, route_s)
    }

    init { () }

    next(state: ()) {
        let (tok, request) = recv(join(), request_r);
        let valid0 = request.valid[0+:u1] == u1:1;
        let valid1 = request.valid[1+:u1] == u1:1;
        let tok = send(tok, route_s, request.valid);
        let tok = send_if(tok, plane0_command_s, valid0, request.commands[u32:0]);
        let tok = send_if(tok, plane1_command_s, valid1, request.commands[u32:1]);
        state
    }
}

proc pair_to_planes_responses {
    route_r: chan<u2> in;
    plane0_response_r: chan<memory::MemoryResponse> in;
    plane1_response_r: chan<memory::MemoryResponse> in;
    response_s: chan<memory::MemoryResponsePair> out;

    config(route_r: chan<u2> in, plane0_response_r: chan<memory::MemoryResponse> in,
           plane1_response_r: chan<memory::MemoryResponse> in,
           response_s: chan<memory::MemoryResponsePair> out) {
        (route_r, plane0_response_r, plane1_response_r, response_s)
    }

    init { () }

    next(state: ()) {
        let (tok, route) = recv(join(), route_r);
        let valid0 = route[0+:u1] == u1:1;
        let valid1 = route[1+:u1] == u1:1;
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

pub proc pair_to_planes {
    config(request_r: chan<memory::MemoryRequestPair> in,
           response_s: chan<memory::MemoryResponsePair> out,
           plane0_command_s: chan<memory::MemoryCommand> out,
           plane0_response_r: chan<memory::MemoryResponse> in,
           plane1_command_s: chan<memory::MemoryCommand> out,
           plane1_response_r: chan<memory::MemoryResponse> in) {
        let (route_s, route_r) = #[channel(depth=32, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                 chan<u2>("route");
        spawn pair_to_planes_requests(request_r, plane0_command_s, plane1_command_s, route_s);
        spawn pair_to_planes_responses(route_r, plane0_response_r, plane1_response_r, response_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc memory_arbiter2_requests {
    request0_r: chan<memory::MemoryCommand> in;
    request1_r: chan<memory::MemoryCommand> in;
    request_s: chan<memory::MemoryCommand> out;
    route_s: chan<u1> out;

    config(request0_r: chan<memory::MemoryCommand> in, request1_r: chan<memory::MemoryCommand> in,
           request_s: chan<memory::MemoryCommand> out, route_s: chan<u1> out) {
        (request0_r, request1_r, request_s, route_s)
    }

    init { () }

    next(state: ()) {
        let (tok, request0, valid0) = recv_non_blocking(join(), request0_r, empty_command());
        let (tok, request1, valid1) = if valid0 {
            (tok, empty_command(), false)
        } else {
            recv_non_blocking(tok, request1_r, empty_command())
        };
        let valid = valid0 || valid1;
        let tok = send_if(tok, route_s, valid, valid1 as u1);
        let tok = send_if(tok, request_s, valid, if valid0 { request0 } else { request1 });
        state
    }
}

proc memory_arbiter2_responses {
    route_r: chan<u1> in;
    response_r: chan<memory::MemoryResponse> in;
    response0_s: chan<memory::MemoryResponse> out;
    response1_s: chan<memory::MemoryResponse> out;

    config(route_r: chan<u1> in, response_r: chan<memory::MemoryResponse> in,
           response0_s: chan<memory::MemoryResponse> out,
           response1_s: chan<memory::MemoryResponse> out) {
        (route_r, response_r, response0_s, response1_s)
    }

    init { () }

    next(state: ()) {
        let (tok, route) = recv(join(), route_r);
        let (tok, response) = recv(tok, response_r);
        let tok = send_if(tok, response0_s, route == u1:0, response);
        let tok = send_if(tok, response1_s, route == u1:1, response);
        state
    }
}

pub proc memory_arbiter2 {
    config(request0_r: chan<memory::MemoryCommand> in,
           response0_s: chan<memory::MemoryResponse> out,
           request1_r: chan<memory::MemoryCommand> in,
           response1_s: chan<memory::MemoryResponse> out,
           request_s: chan<memory::MemoryCommand> out, response_r: chan<memory::MemoryResponse> in) {
        let (route_s, route_r) = #[channel(depth=32, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                 chan<u1>("route");
        spawn memory_arbiter2_requests(request0_r, request1_r, request_s, route_s);
        spawn memory_arbiter2_responses(route_r, response_r, response0_s, response1_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc memory_arbiter3_requests {
    request0_r: chan<memory::MemoryCommand> in;
    request1_r: chan<memory::MemoryCommand> in;
    request2_r: chan<memory::MemoryCommand> in;
    request_s: chan<memory::MemoryCommand> out;
    route_s: chan<u2> out;

    config(request0_r: chan<memory::MemoryCommand> in, request1_r: chan<memory::MemoryCommand> in,
           request2_r: chan<memory::MemoryCommand> in, request_s: chan<memory::MemoryCommand> out,
           route_s: chan<u2> out) {
        (request0_r, request1_r, request2_r, request_s, route_s)
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
        let selected = if valid0 {
            u2:0
        } else if valid1 {
            u2:1
        } else {
            u2:2
        };
        let request = if valid0 {
            request0
        } else if valid1 {
            request1
        } else {
            request2
        };
        let tok = send_if(tok, route_s, valid, selected);
        let tok = send_if(tok, request_s, valid, request);
        state
    }
}

proc memory_arbiter3_responses {
    route_r: chan<u2> in;
    response_r: chan<memory::MemoryResponse> in;
    response0_s: chan<memory::MemoryResponse> out;
    response1_s: chan<memory::MemoryResponse> out;
    response2_s: chan<memory::MemoryResponse> out;

    config(route_r: chan<u2> in, response_r: chan<memory::MemoryResponse> in,
           response0_s: chan<memory::MemoryResponse> out,
           response1_s: chan<memory::MemoryResponse> out,
           response2_s: chan<memory::MemoryResponse> out) {
        (route_r, response_r, response0_s, response1_s, response2_s)
    }

    init { () }

    next(state: ()) {
        let (tok, route) = recv(join(), route_r);
        let (tok, response) = recv(tok, response_r);
        let tok = send_if(tok, response0_s, route == u2:0, response);
        let tok = send_if(tok, response1_s, route == u2:1, response);
        let tok = send_if(tok, response2_s, route == u2:2, response);
        state
    }
}

pub proc memory_arbiter3 {
    config(request0_r: chan<memory::MemoryCommand> in,
           response0_s: chan<memory::MemoryResponse> out,
           request1_r: chan<memory::MemoryCommand> in,
           response1_s: chan<memory::MemoryResponse> out,
           request2_r: chan<memory::MemoryCommand> in,
           response2_s: chan<memory::MemoryResponse> out,
           request_s: chan<memory::MemoryCommand> out, response_r: chan<memory::MemoryResponse> in) {
        let (route_s, route_r) = #[channel(depth=32, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                 chan<u2>("route");
        spawn memory_arbiter3_requests(request0_r, request1_r, request2_r, request_s, route_s);
        spawn memory_arbiter3_responses(route_r, response_r, response0_s, response1_s, response2_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc word_transfer_requests {
    command_r: chan<memory::MemoryCommand> in;
    transfer_s: chan<MemoryTransfer> out;

    config(command_r: chan<memory::MemoryCommand> in, transfer_s: chan<MemoryTransfer> out) {
        (command_r, transfer_s)
    }

    init { () }

    next(state: ()) {
        let (tok, command) = recv(join(), command_r);
        let tok = send(tok, transfer_s, word_transfer(command));
        state
    }
}

proc word_transfer_responses {
    transfer_response_r: chan<MemoryTransferResponse> in;
    response_s: chan<memory::MemoryResponse> out;

    config(transfer_response_r: chan<MemoryTransferResponse> in,
           response_s: chan<memory::MemoryResponse> out) {
        (transfer_response_r, response_s)
    }

    init { () }

    next(state: ()) {
        let (tok, response) = recv(join(), transfer_response_r);
        let tok = send(tok, response_s, word_response(response));
        state
    }
}

pub proc word_transfer_adapter {
    config(command_r: chan<memory::MemoryCommand> in, response_s: chan<memory::MemoryResponse> out,
           transfer_s: chan<MemoryTransfer> out,
           transfer_response_r: chan<MemoryTransferResponse> in) {
        spawn word_transfer_requests(command_r, transfer_s);
        spawn word_transfer_responses(transfer_response_r, response_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc block_read_transfer_requests<WORDS: u7 = {u7:64}> {
    command_r: chan<memory::MemoryBlockRead> in;
    transfer_s: chan<MemoryTransfer> out;

    config(command_r: chan<memory::MemoryBlockRead> in, transfer_s: chan<MemoryTransfer> out) {
        (command_r, transfer_s)
    }

    init { () }

    next(state: ()) {
        let (tok, command) = recv(join(), command_r);
        let tok = send(tok, transfer_s, block_read_transfer<WORDS>(command));
        state
    }
}

proc block_read_transfer_responses {
    transfer_response_r: chan<MemoryTransferResponse> in;
    response_s: chan<memory::MemoryBlockReadWordResponse> out;

    config(transfer_response_r: chan<MemoryTransferResponse> in,
           response_s: chan<memory::MemoryBlockReadWordResponse> out) {
        (transfer_response_r, response_s)
    }

    init { () }

    next(state: ()) {
        let (tok, response) = recv(join(), transfer_response_r);
        let tok = send(tok, response_s, block_read_response(response));
        state
    }
}

pub proc block_read_transfer_adapter<WORDS: u7 = {u7:64}> {
    config(command_r: chan<memory::MemoryBlockRead> in,
           response_s: chan<memory::MemoryBlockReadWordResponse> out,
           transfer_s: chan<MemoryTransfer> out,
           transfer_response_r: chan<MemoryTransferResponse> in) {
        spawn block_read_transfer_requests<WORDS>(command_r, transfer_s);
        spawn block_read_transfer_responses(transfer_response_r, response_s);
        ()
    }

    init { () }

    next(state: ()) { state }
}

proc transfer_arbiter2_requests {
    request0_r: chan<MemoryTransfer> in;
    request1_r: chan<MemoryTransfer> in;
    request_s: chan<MemoryTransfer> out;
    route_s: chan<TransferRoute> out;

    config(request0_r: chan<MemoryTransfer> in, request1_r: chan<MemoryTransfer> in,
           request_s: chan<MemoryTransfer> out, route_s: chan<TransferRoute> out) {
        (request0_r, request1_r, request_s, route_s)
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
        let response_count = if request.command.write == u1:1 || request.word_count == u7:0 {
            u7:1
        } else {
            request.word_count
        };
        let route = TransferRoute { client: valid1 as u1, response_count };
        let tok = send_if(tok, route_s, valid, route);
        let tok = send_if(tok, request_s, valid, request);
        state
    }
}

proc transfer_arbiter2_responses {
    route_r: chan<TransferRoute> in;
    response_r: chan<MemoryTransferResponse> in;
    response0_s: chan<MemoryTransferResponse> out;
    response1_s: chan<MemoryTransferResponse> out;

    config(route_r: chan<TransferRoute> in, response_r: chan<MemoryTransferResponse> in,
           response0_s: chan<MemoryTransferResponse> out,
           response1_s: chan<MemoryTransferResponse> out) {
        (route_r, response_r, response0_s, response1_s)
    }

    init { TransferRoute { client: u1:0, response_count: u7:0 } }

    next(state: TransferRoute) {
        let idle = state.response_count == u7:0;
        let (tok, incoming) = recv_if(
            join(), route_r, idle, TransferRoute { client: u1:0, response_count: u7:1 });
        let route = if idle { incoming } else { state };
        let (tok, response) = recv(tok, response_r);
        let tok = send_if(tok, response0_s, route.client == u1:0, response);
        let tok = send_if(tok, response1_s, route.client == u1:1, response);
        TransferRoute { response_count: route.response_count - u7:1, ..route }
    }
}

pub proc transfer_arbiter2 {
    config(request0_r: chan<MemoryTransfer> in, response0_s: chan<MemoryTransferResponse> out,
           request1_r: chan<MemoryTransfer> in, response1_s: chan<MemoryTransferResponse> out,
           request_s: chan<MemoryTransfer> out, response_r: chan<MemoryTransferResponse> in) {
        let (route_s, route_r) = #[channel(depth=2, bypass=false, register_push_outputs=true, register_pop_outputs=false)]
                                 chan<TransferRoute>("route");
        spawn transfer_arbiter2_requests(request0_r, request1_r, request_s, route_s);
        spawn transfer_arbiter2_responses(route_r, response_r, response0_s, response1_s);
        ()
    }

    init { () }

    next(state: ()) { state }
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
    let response = MemoryTransferResponse { data: u32:0xdeadbeef, success: u1:1 };
    assert_eq(
        word_response(response), memory::MemoryResponse { data: u32:0xdeadbeef, success: u1:1 });
    assert_eq(block_read_response(response).data, u32:0xdeadbeef);
}

#[test_proc]
proc memory_arbiter2_test {
    request0_s: chan<memory::MemoryCommand> out;
    response0_r: chan<memory::MemoryResponse> in;
    request1_s: chan<memory::MemoryCommand> out;
    response1_r: chan<memory::MemoryResponse> in;
    request_r: chan<memory::MemoryCommand> in;
    response_s: chan<memory::MemoryResponse> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (request0_s, request0_r) = chan<memory::MemoryCommand>("request0");
        let (response0_s, response0_r) = chan<memory::MemoryResponse>("response0");
        let (request1_s, request1_r) = chan<memory::MemoryCommand>("request1");
        let (response1_s, response1_r) = chan<memory::MemoryResponse>("response1");
        let (request_s, request_r) = chan<memory::MemoryCommand>("request");
        let (response_s, response_r) = chan<memory::MemoryResponse>("response");
        spawn memory_arbiter2(
            request0_r, response0_s, request1_r, response1_s, request_s, response_r);
        (request0_s, response0_r, request1_s, response1_r, request_r, response_s, terminator)
    }

    init { () }

    next(state: ()) {
        let command0 =
            memory::MemoryCommand { write: u1:0, address: u22:7, data: u32:0, mask: u4:0 };
        let command1 = memory::MemoryCommand { address: u22:8, ..command0 };
        let tok = send(join(), request1_s, command1);
        let (tok, forwarded1) = recv(tok, request_r);
        assert_eq(forwarded1, command1);
        let expected1 = memory::MemoryResponse { data: u32:0x12345678, success: u1:1 };
        let tok = send(tok, response_s, expected1);
        let (tok, response1) = recv(tok, response1_r);
        assert_eq(response1, expected1);
        let tok = send(tok, request0_s, command0);
        let (tok, forwarded0) = recv(tok, request_r);
        assert_eq(forwarded0, command0);
        let expected0 = memory::MemoryResponse { data: u32:0xcafebabe, success: u1:1 };
        let tok = send(tok, response_s, expected0);
        let (tok, response0) = recv(tok, response0_r);
        assert_eq(response0, expected0);
        send(tok, terminator, true);
    }
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
        let command0 =
            memory::MemoryCommand { write: u1:0, address: u22:7, data: u32:0, mask: u4:0 };
        let command1 = memory::MemoryCommand { address: u22:8, ..command0 };
        let tok = send(join(), request1_s, command0);
        let (tok, forwarded0) = recv(tok, request_r);
        assert_eq(forwarded0, command0);
        let tok = send(tok, request1_s, command1);
        let (tok, forwarded1) = recv(tok, request_r);
        assert_eq(forwarded1, command1);
        let expected0 = memory::MemoryResponse { data: u32:0xcafebabe, success: u1:1 };
        let expected1 = memory::MemoryResponse { data: u32:0x12345678, success: u1:1 };
        let tok = send(tok, response_s, expected0);
        let (tok, response0) = recv(tok, response1_r);
        assert_eq(response0, expected0);
        let tok = send(tok, response_s, expected1);
        let (tok, response1) = recv(tok, response1_r);
        assert_eq(response1, expected1);
        send(tok, terminator, true);
    }
}

#[test_proc]
proc word_transfer_adapter_test {
    command_s: chan<memory::MemoryCommand> out;
    response_r: chan<memory::MemoryResponse> in;
    transfer_r: chan<MemoryTransfer> in;
    transfer_response_s: chan<MemoryTransferResponse> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (command_s, command_r) = chan<memory::MemoryCommand>("command");
        let (response_s, response_r) = chan<memory::MemoryResponse>("response");
        let (transfer_s, transfer_r) = chan<MemoryTransfer>("transfer");
        let (transfer_response_s, transfer_response_r) =
            chan<MemoryTransferResponse>("transfer_response");
        spawn word_transfer_adapter(command_r, response_s, transfer_s, transfer_response_r);
        (command_s, response_r, transfer_r, transfer_response_s, terminator)
    }

    init { () }

    next(state: ()) {
        let command0 =
            memory::MemoryCommand { write: u1:0, address: u22:3, data: u32:0, mask: u4:0xf };
        let command1 = memory::MemoryCommand { address: u22:4, ..command0 };
        let tok = send(join(), command_s, command0);
        let (tok, transfer0) = recv(tok, transfer_r);
        assert_eq(transfer0, word_transfer(command0));
        let tok = send(tok, command_s, command1);
        let (tok, transfer1) = recv(tok, transfer_r);
        assert_eq(transfer1, word_transfer(command1));
        let expected0 = MemoryTransferResponse { data: u32:0x11111111, success: u1:1 };
        let expected1 = MemoryTransferResponse { data: u32:0x22222222, success: u1:1 };
        let tok = send(tok, transfer_response_s, expected0);
        let (tok, response0) = recv(tok, response_r);
        assert_eq(response0, word_response(expected0));
        let tok = send(tok, transfer_response_s, expected1);
        let (tok, response1) = recv(tok, response_r);
        assert_eq(response1, word_response(expected1));
        send(tok, terminator, true);
    }
}

#[test_proc]
proc transfer_arbiter2_test {
    request0_s: chan<MemoryTransfer> out;
    response0_r: chan<MemoryTransferResponse> in;
    request1_s: chan<MemoryTransfer> out;
    response1_r: chan<MemoryTransferResponse> in;
    request_r: chan<MemoryTransfer> in;
    response_s: chan<MemoryTransferResponse> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (request0_s, request0_r) = chan<MemoryTransfer>("request0");
        let (response0_s, response0_r) = chan<MemoryTransferResponse>("response0");
        let (request1_s, request1_r) = chan<MemoryTransfer>("request1");
        let (response1_s, response1_r) = chan<MemoryTransferResponse>("response1");
        let (request_s, request_r) = chan<MemoryTransfer>("request");
        let (response_s, response_r) = chan<MemoryTransferResponse>("response");
        spawn transfer_arbiter2(
            request0_r, response0_s, request1_r, response1_s, request_s, response_r);
        (request0_s, response0_r, request1_s, response1_r, request_r, response_s, terminator)
    }

    init { () }

    next(state: ()) {
        let command0 =
            memory::MemoryCommand { write: u1:0, address: u22:5, data: u32:0, mask: u4:0xf };
        let command1 = memory::MemoryCommand { address: u22:6, ..command0 };
        let request0 = MemoryTransfer { word_count: u7:3, command: command0 };
        let request1 = word_transfer(command1);
        let tok = send(join(), request0_s, request0);
        let (tok, forwarded0) = recv(tok, request_r);
        assert_eq(forwarded0, request0);
        let tok = send(tok, request1_s, request1);
        let (tok, forwarded1) = recv(tok, request_r);
        assert_eq(forwarded1, request1);
        let expected0 = MemoryTransferResponse { data: u32:0xaaaaaaaa, success: u1:1 };
        let expected1 = MemoryTransferResponse { data: u32:0xbbbbbbbb, success: u1:1 };
        let tok = unroll_for! (_, tok): (u2, token) in u2:0..u2:3 {
            let tok = send(tok, response_s, expected0);
            let (tok, response0) = recv(tok, response0_r);
            assert_eq(response0, expected0);
            tok
        }(tok);
        let tok = send(tok, response_s, expected1);
        let (tok, response1) = recv(tok, response1_r);
        assert_eq(response1, expected1);
        send(tok, terminator, true);
    }
}

#[test_proc]
proc pair_to_planes_test {
    request_s: chan<memory::MemoryRequestPair> out;
    response_r: chan<memory::MemoryResponsePair> in;
    plane0_command_r: chan<memory::MemoryCommand> in;
    plane0_response_s: chan<memory::MemoryResponse> out;
    plane1_command_r: chan<memory::MemoryCommand> in;
    plane1_response_s: chan<memory::MemoryResponse> out;
    terminator: chan<bool> out;

    config(terminator: chan<bool> out) {
        let (request_s, request_r) = chan<memory::MemoryRequestPair>("request");
        let (response_s, response_r) = chan<memory::MemoryResponsePair>("response");
        let (plane0_command_s, plane0_command_r) = chan<memory::MemoryCommand>("plane0_command");
        let (plane0_response_s, plane0_response_r) =
            chan<memory::MemoryResponse>("plane0_response");
        let (plane1_command_s, plane1_command_r) = chan<memory::MemoryCommand>("plane1_command");
        let (plane1_response_s, plane1_response_r) =
            chan<memory::MemoryResponse>("plane1_response");
        spawn pair_to_planes(
            request_r, response_s, plane0_command_s, plane0_response_r, plane1_command_s,
            plane1_response_r);
        (
            request_s, response_r, plane0_command_r, plane0_response_s, plane1_command_r,
            plane1_response_s, terminator,
        )
    }

    init { () }

    next(state: ()) {
        let command0 =
            memory::MemoryCommand { write: u1:0, address: u22:9, data: u32:0, mask: u4:0xf };
        let command1 = memory::MemoryCommand { address: u22:10, ..command0 };
        let empty = empty_command();
        let request0 = memory::MemoryRequestPair {
            valid: u2:1,
            commands: memory::MemoryCommand[2]:[command0, empty],
        };
        let request1 = memory::MemoryRequestPair {
            valid: u2:2,
            commands: memory::MemoryCommand[2]:[empty, command1],
        };
        let tok = send(join(), request_s, request0);
        let (tok, forwarded0) = recv(tok, plane0_command_r);
        assert_eq(forwarded0, command0);
        let tok = send(tok, request_s, request1);
        let (tok, forwarded1) = recv(tok, plane1_command_r);
        assert_eq(forwarded1, command1);
        let expected0 = memory::MemoryResponse { data: u32:0x11111111, success: u1:1 };
        let expected1 = memory::MemoryResponse { data: u32:0x22222222, success: u1:1 };
        let tok = send(tok, plane0_response_s, expected0);
        let (tok, response0) = recv(tok, response_r);
        assert_eq(response0.responses[u32:0], expected0);
        let tok = send(tok, plane1_response_s, expected1);
        let (tok, response1) = recv(tok, response_r);
        assert_eq(response1.responses[u32:1], expected1);
        send(tok, terminator, true);
    }
}
