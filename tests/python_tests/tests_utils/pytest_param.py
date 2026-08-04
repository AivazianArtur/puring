from collections.abc import Iterable, Sequence
from dataclasses import dataclass, field
from typing import Any

import pytest
from _pytest.mark.structures import ParameterSet


@dataclass(frozen=True)
class KwargsParameterSet:
    """ParameterSet с поддержкой kwargs."""

    kwargs_values: dict
    marks: Any = field(default_factory=tuple)
    id: str | None = None

    def to_parameter_set(self, arg_names: Sequence[str]) -> ParameterSet:
        values: list = []
        for arg_name in arg_names:
            if arg_name in self.kwargs_values:
                values.append(self.kwargs_values.pop(arg_name))
            else:
                err_msg = (
                    f'Parametrize parameter for "{arg_name}" was not provided '
                    f'in parameter set "{self.kwargs_values}"',
                )
                raise RuntimeError(err_msg)

        if self.kwargs_values:
            err_msg = (
                f'Found orphan parameters in parameter set: {self.kwargs_values}\n'
                f'Available keys: {arg_names}\n'
                'Remove them to continue test',
            )
            raise RuntimeError(err_msg)

        return pytest.param(
            *values,
            marks=self.marks,
            id=self.id,
        )


def pytest_parametrize(
    argnames: str | Sequence[str],
    argvalues: Iterable[ParameterSet | KwargsParameterSet | Sequence[object] | object],
    **kwargs: Any,
):
    if isinstance(argnames, str):
        argnames = [arg_name.strip() for arg_name in argnames.split(',')]

    new_arg_values = []
    for arg_value in argvalues:
        if isinstance(arg_value, KwargsParameterSet):
            arg_value = arg_value.to_parameter_set(argnames)
        new_arg_values.append(arg_value)

    return pytest.mark.parametrize(
        argnames,
        new_arg_values,
        **kwargs,
    )


def pytest_param(**kwargs: Any) -> KwargsParameterSet:
    return KwargsParameterSet(
        marks=kwargs.pop('marks', ()),
        id=kwargs.pop('id', None),
        kwargs_values=kwargs,
    )
