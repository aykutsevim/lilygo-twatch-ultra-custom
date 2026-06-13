import type { ConnStatus } from "../types";

const LABEL: Record<ConnStatus, string> = {
  connecting: "Connecting…",
  online: "Live",
  reconnecting: "Reconnecting…",
  offline: "Offline",
};

interface Props {
  status: ConnStatus;
  remaining: number;
  doneCount: number;
  onClearDone: () => void;
}

export function StatusBar({ status, remaining, doneCount, onClearDone }: Props) {
  return (
    <div className="status">
      <span className={`dot ${status}`} aria-live="polite" title={LABEL[status]}>
        ● {LABEL[status]}
      </span>
      <span className="count">{remaining} left</span>
      <button disabled={doneCount === 0} onClick={onClearDone}>
        Clear completed{doneCount ? ` (${doneCount})` : ""}
      </button>
    </div>
  );
}
