"""Access to an iotsaDataLogger device, through the iotsa package."""
from typing import List, Optional, Tuple

import iotsa
import requests

from .annotations import Annotations
from .records import DailyHistory, RawReading, read_daily_csv, read_raw_csv


def device_name(target: str) -> str:
    """The iotsa device name for a target: ``accugroot.local`` -> ``accugroot``."""
    return target[:-len(".local")] if target.endswith(".local") else target


def device_host(target: str) -> str:
    """The host to contact for a target: a bare device name gets ``.local`` added."""
    return target if "." in target else target + ".local"


class DataLoggerDevice:
    def __init__(
        self,
        target: str,
        protocol: Optional[str] = None,
        port: Optional[int] = None,
        noverify: bool = False,
        bearer: Optional[str] = None,
        auth: Optional[Tuple[str, str]] = None,
        verbose: bool = False,
        timeout: float = 15,
    ):
        self.name = device_name(target)
        self.host = device_host(target)
        self.verbose = verbose
        self.timeout = timeout
        self.device = iotsa.IotsaDevice(
            self.host, protocol=protocol, port=port, noverify=noverify, bearer=bearer, auth=auth
        )

    def fetch_raw(self) -> List[RawReading]:
        """The device's recent full-resolution readings (its raw retention window)."""
        return read_raw_csv(self._get_csv("datalogger/data.csv"))

    def fetch_daily(self) -> DailyHistory:
        """The device's daily summaries, including on-the-fly ones for days still in raw form."""
        return read_daily_csv(self._get_csv("datalogger/data_daily.csv"))

    def fetch_annotations(self) -> Optional[Annotations]:
        """The device's annotations, or None if its firmware has no annotations module."""
        try:
            reply = self.device.protocolHandler.get("annotations")
        except requests.exceptions.HTTPError as e:
            if e.response is not None and e.response.status_code == 404:
                return None
            raise
        return reply or {}

    def set_annotations(self, annotations: Annotations) -> None:
        """Merge annotations into the device's annotations. An empty value removes a key."""
        self.device.protocolHandler.put("annotations", json=annotations)

    def _get_csv(self, path: str) -> List[str]:
        # The iotsa protocol handlers only return JSON replies, so for the CSV
        # endpoints we build the request ourselves from the handler's settings.
        # Only works for the http/https handlers. Workaround, see
        # cwi-dis/iotsaDataLogger#13, blocked on cwi-dis/iotsa#289.
        ph = self.device.protocolHandler
        url = ph.baseURL + path
        headers = {}
        if ph.bearer:
            headers["Authorization"] = "Bearer " + ph.bearer
        if self.verbose:
            print(f"Fetching {url}")
        response = requests.get(
            url, auth=ph.auth, verify=not ph.noverify, headers=headers, timeout=self.timeout
        )
        response.raise_for_status()
        return response.text.splitlines()
