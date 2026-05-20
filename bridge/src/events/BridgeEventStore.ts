import type { BridgeInputEvent } from "../stt/SttTypes.js";

export class BridgeEventStore {
  private nextId = 1001;
  private readonly events: BridgeInputEvent[] = [];

  addSttEvent(create: (id: number) => BridgeInputEvent): BridgeInputEvent {
    const event = create(this.nextId++);
    this.events.push(event);
    return event;
  }

  list(): BridgeInputEvent[] {
    return [...this.events];
  }

  latest(): BridgeInputEvent | null {
    return this.events.at(-1) ?? null;
  }

  clear(): void {
    this.events.length = 0;
  }
}
